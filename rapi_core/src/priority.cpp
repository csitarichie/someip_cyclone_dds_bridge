
#include <iostream>

#include "adstutil_cxx/error_handler.hpp"

#include <fmt/format.h>
#include "core/actor.hpp"
#include "core/priority.hpp"

using Error           = adst::common::Error;
using OnErrorCallBack = adst::common::OnErrorCallBack;

using Priority = adst::ep::test_engine::core::Priority;

Priority::Priority(int dispatcherCount, const OnErrorCallBack& onErrorCallBack)
    : onErrorCallBack_(onErrorCallBack)
    , startCount_(dispatcherCount)
{
    ADSTLOG_INIT_ACTOR_TRACE_MODULES("Priority");
    for (int count = 0; count < dispatcherCount; ++count) {
        auto dispatcherName = fmt::format("dispatcher[{}]", count);
        LOG_C_D("starting '%s'", dispatcherName.c_str());
        dispatchers_.emplace_back([this, count] { dispatcherLoop(count); });
    }
}

void Priority::dispatcherLoop(int threadId)
{
    auto dispatcherName = fmt::format("dispatcher[{}]", threadId);
    ADSTLOG_REGISTER_THREAD(0, dispatcherName.c_str());
    std::unique_lock<std::mutex> startLock(stateChangeMutex_);
    if (state_ != State::RUNNING) {
        startEvent_.wait(startLock, [this] { return state_ == State::RUNNING; });
    }
    startLock.unlock();
    {
        std::lock_guard<std::mutex> countGuard(startedMutex_);
        startCount_--;
    }
    startedEvent_.notify_one();
    LOG_C_D("dispatcher[%d] started", threadId);
    std::unique_lock<std::mutex> actorQueueLock(stateChangeMutex_);
    while (state_ == State::RUNNING || !actorQueue_.empty()) {
        if (actorQueue_.empty()) {
            // there could be 2 reason thread wakeup either there is an actor to be processed.
            // or the priority level stopped
            sleepingCount_++;
            if (sleepingCount_ == dispatchers_.size()) {
                idleEvent_.notify_one();
            }
            scheduleEvent_.wait(actorQueueLock, [this] { return (!actorQueue_.empty() || state_ != State::RUNNING); });
            sleepingCount_--;
        }

        // take an actor from the queue and start process it's queue
        if (!actorQueue_.empty()) {
            auto callBack = actorQueue_.front();
            actorQueue_.pop_front();
            // now actually do the job.
            LOG_C_D("scheduled callback on dispatcher[%d]", threadId);
            Actor::thread_id = threadId;
            actorQueueLock.unlock();
            callBack();
            actorQueueLock.lock();
        }
    }
    sleepingCount_++;
    startedEvent_.notify_one(); // unlock stop (reusing started event)
}

void Priority::waitForIdle()
{
    std::unique_lock<std::mutex> actorQueueLock(stateChangeMutex_);
    if (!actorQueue_.empty() || sleepingCount_ != dispatchers_.size()) {
        idleEvent_.wait(actorQueueLock, [this] { return sleepingCount_ == dispatchers_.size(); });
    }
}

Priority::~Priority()
{
    {
        std::unique_lock<std::mutex> stateLock(stateChangeMutex_);
        switch (state_) {
            case State::START:
                start(stateLock);
                stateLock.lock();
                if (state_ == State::RUNNING) {
                    stop(stateLock);
                }
                break;
            case State::RUNNING:
                stop(stateLock);
                break;
            case State::STOPPING:
                onErrorCallBack_(Error{{3, "priority destructor and stop must be called from the same thread"}});
                break;
            case State::STOPPED:
                break;
            default:
                onErrorCallBack_(Error{{3, "unknown priority state"}});
                break;
        }
    }
    for (auto& thread : dispatchers_) {
        thread.join();
        ADSTLOG_UNREGISTER_THREAD(thread.native_handle());
    }
}

void Priority::start()
{
    std::unique_lock<std::mutex> runningLock(stateChangeMutex_);
    start(runningLock);
}

void Priority::start(std::unique_lock<std::mutex>& lock)
{
    if (state_ != State::START) {
        onErrorCallBack_(Error{{4, "Priority::start can only be called once"}});
    }
    state_ = State::RUNNING;
    lock.unlock();
    LOG_C_D("running_ = RUNNING");
    startEvent_.notify_all();
    std::unique_lock<std::mutex> startLock(startedMutex_);
    startedEvent_.wait(startLock, [this] { return startCount_ == 0; });
}

void Priority::stop()
{
    LOG_C_D("stop");
    std::unique_lock<std::mutex> runningLock(stateChangeMutex_);
    stop(runningLock);
}

void Priority::stop(std::unique_lock<std::mutex>& lock)
{
    if (state_ != State::RUNNING) {
        onErrorCallBack_(Error{{2, "Priority::stop without prior start"}});
    }
    state_ = State::STOPPING;
    LOG_C_D("running_ = STOPPING");
    if (!actorQueue_.empty() || sleepingCount_ != dispatchers_.size()) {
        LOG_C_D("waiting to finish: queue.size=%d, sleepingCount=%d", actorQueue_.size(), sleepingCount_);
        startedEvent_.wait(lock, [this] { return (actorQueue_.empty() && sleepingCount_ == dispatchers_.size()); }); //
        // started event reused.
    }
    state_ = State::STOPPED;
    LOG_C_D("running_ = STOPPED");
    lock.unlock();
    scheduleEvent_.notify_all();
}

void Priority::schedule(CallBack callBack)
{
    std::lock_guard<std::mutex> actorQueueGuard(stateChangeMutex_);
    if (state_ == State::STOPPED) {
        onErrorCallBack_(Error{{3, "Priority::schedule after STOPPED state reached"}});
    }
    actorQueue_.push_back(std::move(callBack));
    if (actorQueue_.size() == 1) {
        scheduleEvent_.notify_all();
    }
}
