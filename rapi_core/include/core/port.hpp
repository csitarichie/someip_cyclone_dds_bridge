#pragma once

#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <deque>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <mutex>

#include <iostream>

#include "adstlog_cxx/adstlog.hpp"
#include "adstutil_cxx/compiler_diagnostics.hpp"
#include "core/impl/async_call_back_vector.hpp"
#include "core/impl/common.hpp"
#include "core/network.hpp"

namespace adst::ep::test_engine::core {

class Actor;

/**
 * Used by an actor to represent an incoming entity.
 *
 * Listen on the port registers the actor in the network with the given event.
 *
 * Handles event callback dispatching when an event is received by an actor from a network.
 */
class Port
{
public:
    /**
     * ctor.
     *
     * A port is always owned by an actor.
     *
     * Port schedules itself on an actor when there is a command or event to dispatch.
     *
     * The actor schedule function makes sure that the actor only exists once in the dispatching
     * queue of the priority.
     *
     * @param actor Actor which ownes this port.
     * @param network The network this port is connected to.
     */
    explicit Port(Actor& actor, Network& network);

    ~Port()
    {
        std::lock_guard<std::mutex> guard{callbacksMutex_};
        callbacks_.clear();
    }

    Port()            = delete;
    Port(const Port&) = delete;
    Port(Port&&)      = delete;

    Port& operator=(Port&&) = delete;
    Port& operator=(const Port&) = delete;

    /**
     * Creates unique handle for each listen registration.
     *
     * @return A unique handle.
     */
    CallBackHandle newHandle()
    {
        std::lock_guard<std::mutex> guard{eventMutex_};
        auto                        handle = ++handleCounter_;
        return handle;
    }

    /**
     * Used by the actor to register a callback to a given port (network).
     *
     * @tparam Event The event type used for callback (and for registration).
     * @param callback The callback to be called when a publish happens on a network.
     * @return Unique ID to identify the callback during unlisten.
     */
    template <typename Event>
    CallBackHandle listen(std::function<void(const Event&)> callback)
    {
        static_assert(impl::validateEvent<Event>(), "Invalid event");

        auto handle = newHandle();
        listen<Event>(handle, std::move(callback));
        return handle;
    }

    /**
     * Listen can be changed to a different function by providing the unique id and a new callback.
     *
     * @tparam Event the event type used for dispatch.
     * @param handle Unique handle of the initial listen.
     * @param callback The callback to register under for handle.
     */
    template <typename Event>
    void listen(const CallBackHandle handle, std::function<void(const Event&)> callback);

    /**
     * Removes all callback from all event type with a given handle.
     *
     * @param handle Unique handle of the initial listen.
     */
    void unlistenAll(const CallBackHandle handle)
    {
        std::lock_guard<std::mutex> unListenCmdQueueGuard{eventMutex_};
        commandsQueue_.emplace_back([this, handle]() {
            std::lock_guard<std::mutex> lambdaCallbacksGuard{callbacksMutex_};
            for (auto& element : callbacks_) {
                element.second->remove(handle);
            }
        });
    }

    /**
     * Removes callback from the listen container.
     *
     * @tparam Event Type of the event used in the callback.
     * @param handle Unique id (from the listen call).
     */
    template <typename Event>
    void unlisten(CallBackHandle handle);

    /**
     * Called by the the Actor when it has a message to be dispatched.
     *
     * @tparam Event The type of the message.
     * @param event A unique_ptr to Event/Message.
     */
    template <typename Event>
    void schedule(Event event); // ToDo type it to unique ptr.

    /**
     * Allows direct message dispatch to self. Not used for now.
     *
     * @tparam Event The event type to dispatch.
     * @param event A unique_ptr to Event/Message.
     */
    template <typename Event>
    void notify(const Event& event)
    {
        schedule(event);
        consume(1);
    }

    /**
     * Consumes all commands and event from the queue which was pushed by schedule to the port.
     *
     * @param max might be that not all event needs to be dispatched so a maximum number of event to dispatch
     * @return the number of event has been dispatched.
     */
    int consume(int max = std::numeric_limits<int>::max());

    /**
     * The number of events in the queue (waiting to be dispatched).
     *
     * @return size of the event queue
     */
    std::size_t getQueueEventCount() const
    {
        std::lock_guard<std::mutex> guard{eventMutex_};
        return eventQueue_.size();
    }

private:
    /**
     * Stores the AsyncCallBackVector by base class CallbackVector event is hidden.
     */
    using CallBackVector = std::unique_ptr<impl::CallbackVector>;

    /**
     * Mapping of type_id and CallBackVector is used at runtime to get back the callbacks for a
     * specific event type.
     */
    using TypeIdToCallBackVector = std::map<impl::type_id_t, CallBackVector>;

    /**
     * Helper to process the CommandQueue and return the event queue size as a single operation
     * used in a while loop during consume
     *
     * Since the logic is first process all Commands (CallBack manipulation) then process the
     * events.
     *
     * @return Size of the event queue.
     */
    std::size_t processCommandsAndGetQueuedEventsCount();

    /**
     * If Port has either Command or Message to dispatch it schedules itself on the Actor.
     *
     * All prot activity is processed in actor context.
     */
    void scheduleOnOwner(); // must be called from locked queue mutex content; ToDo RSZRSZ Add enforcement.

    CallBackHandle         handleCounter_ = 0;  /// used as a counter to generate unique callback handles
    TypeIdToCallBackVector callbacks_     = {}; /// the store for the callbacks registered for an event
    mutable std::mutex     callbacksMutex_;     /// guard for the command queue
    mutable std::mutex     eventMutex_;         /// quard for the event queue

    /**
     * Queue stores all events for dispatching
     */
    std::deque<std::function<void()>> eventQueue_;

    /**
     * Queue stores all commands for dispatching
     */
    std::deque<std::function<void()>> commandsQueue_;
    Actor&   owner_;   // it is a reference because ports are owned by actor and port schedules itself on an actor.
    Network& network_; // it is now a reference it probably ones dynamic connections are allowed must change to some

    /**
     * A state has to be maintained that the port is already in the actors queue. If it is there is
     * no need to add to it since it is already waiting for scheduling.
     */
    bool scheduled_ = false;
    ADSTLOG_DEF_ACTOR_TRACE_MODULES();
};
} // namespace adst::ep::test_engine::core

#include "core/actor.hpp"

namespace adst::ep::test_engine::core {

template <typename Event>
void Port::listen(const CallBackHandle handle, std::function<void(const Event&)> callback)
{
    static_assert(impl::validateEvent<Event>(), "Invalid event");

    std::lock_guard<std::mutex> listenCmdQueueGuard{eventMutex_};

    commandsQueue_.push_back([this, handle, callback = std::move(callback)]() {
        // it is not shadowing since the lambda is not executed in the listen call context

        std::lock_guard<std::mutex> lambdaCallbacksGuard{callbacksMutex_};

        using Vector = impl::AsyncCallbackVector<Event>;
        // GCOVR_EXCL_START
        assert(callback && "callback should be valid"); // Check for valid object
                                                        // GCOVR_EXCL_STOP
        auto& vector = callbacks_[impl::type_id<Event>()];
        // GCOVR_EXCL_START
        if (vector == nullptr) {
            // GCOVR_EXCL_STOP
            vector.reset(new Vector{});
            network_.listen<Event>([this](const std::shared_ptr<Event> event) { schedule(event); });
        }
        //            assert(dynamic_cast<Vector*>(vector.get())); // ToDo RSZRSZ Create ifdef for RTTI enabled ...
        auto callbacks = static_cast<Vector*>(vector.get());
        LOG_C_D("Port::listen:{%s}", impl::getTypeName<Event>().c_str());
        callbacks->add(handle, callback);
    });
    scheduleOnOwner();
}

template <typename Event>
void Port::schedule(Event event)
{
    using EventT = typename std::remove_reference<decltype(*Event())>::type;
    static_assert(impl::validateEvent<EventT>(), "Invalid event");
    std::shared_ptr<EventT>     shareableEvent = std::move(event);
    std::lock_guard<std::mutex> scheduleCmdQueueGuard{eventMutex_};
    LOG_C_D("Port::schedule, dispatch:{%s}", impl::getTypeName<EventT>().c_str());
    eventQueue_.push_back([this, event = std::move(shareableEvent)]() {
        std::lock_guard<std::mutex> lambdaCallbacksGuard{callbacksMutex_};

        using Vector = impl::AsyncCallbackVector<EventT>;
        auto found   = callbacks_.find(impl::type_id<EventT>());
        LOG_C_D("Port::schedule, dispatch:{%s}", impl::getTypeName<EventT>().c_str());
        // GCOVR_EXCL_START
        if (found == callbacks_.end()) {
            // GCOVR_EXCL_STOP
            LOG_C_D("Port::schedule, no listener:{%s}", impl::getTypeName<EventT>().c_str());
            return; // no such notifications
        }
        std::unique_ptr<impl::CallbackVector>& vector = found->second;
        //            assert(dynamic_cast<Vector*>(vector.get()));
        auto callbacks = static_cast<Vector*>(vector.get());
        LOG_C_D("Port::schedule, dispatch:{%s}, callbacks.size=%d", impl::getTypeName<EventT>().c_str(),
                callbacks->container_.size());
        for (const auto& element : callbacks->container_) {
            LOG_CH_D(MSG_RX, "%s", impl::getTypeName<EventT>().c_str());
            element.second(*event);
        }
    });
    scheduleOnOwner();
}

template <typename Event>
void Port::unlisten(const CallBackHandle handle)
{
    static_assert(impl::validateEvent<Event>(), "Invalid event");
    std::lock_guard<std::mutex> unListenCmdQueueGuard{eventMutex_};
    commandsQueue_.push_back([this, handle]() {
        std::lock_guard<std::mutex> lambdaCallbacksGuard{callbacksMutex_};

        auto found = callbacks_.find(impl::type_id<Event>());
        if (found != callbacks_.end()) {
            LOG_C_D("Port::unlisten:{%s}", impl::getTypeName<Event>().c_str());
            found->second->remove(handle); // ToDo RSZRSZ unlisten from Network.
        }
    });
    scheduleOnOwner();
}

} // namespace adst::ep::test_engine::core
