
#include "core/port.hpp"

using Port = adst::ep::test_engine::core::Port;

Port::Port(Actor& actor, Network& network)
    : owner_(actor)
    , network_(network)
{
    ADSTLOG_INIT_ACTOR_TRACE_MODULES(owner_.getName().c_str());
}

std::size_t Port::processCommandsAndGetQueuedEventsCount()
{
    std::unique_lock<std::mutex> eventCmdQueueLock{eventMutex_};
    std::function<void()>        command;
    while (!commandsQueue_.empty()) {
        // This can't add any extra commands, because in this queue we story only listen/unlisten commands.
        command = commandsQueue_.front();
        commandsQueue_.pop_front();
        // If other thread starts schedule from network and there is a listen / unlisten executed from.
        // here on the same network then it would end in a deadlock if the network could not call schedule.
        eventCmdQueueLock.unlock();
        command();
        eventCmdQueueLock.lock();
    }
    // Yeah we want to return events count. So don't have to call getQueueEventCount
    return eventQueue_.size();
}

int Port::consume(int max)
{
    int consumed = 0;
    LOG_C_D("Port::consume, commandsQueue.size=%d, eventQueue_.size=%d", commandsQueue_.size(), eventQueue_.size());
    std::function<void()> eventCommand;
    while (processCommandsAndGetQueuedEventsCount() > 0 && consumed < max) // order is important
    {
        {
            std::lock_guard<std::mutex> guard{eventMutex_};
            eventCommand = std::move(eventQueue_.front());
            eventQueue_.pop_front();
        }

        eventCommand();
        ++consumed;
    }
    {
        std::lock_guard<std::mutex> guard{eventMutex_};
        scheduled_ = false;
        if (!eventQueue_.empty() || !commandsQueue_.empty()) {
            scheduleOnOwner();
        }
    }

    return consumed;
}

void Port::scheduleOnOwner()
{
    if (!scheduled_) {
        scheduled_ = true;
        owner_.schedule([this] { consume(); });
    }
}
