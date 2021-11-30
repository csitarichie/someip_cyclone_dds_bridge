#pragma once

#include <cassert>
#include <iostream>
#include <map>
#include <mutex>
#include "adstlog_cxx/adstlog.hpp"
#include "core/impl/async_call_back_vector.hpp"
#include "core/impl/common.hpp"

namespace adst::ep::test_engine::core {
/**
 * A network implements a broadcast medium between actor ports.
 *
 * Actors are registering when they listen for a message on a port to a network. So when that
 * message is published to the network actors will be scheduled for dispatch by the priority.
 */
class Network
{
public:
    ~Network()
    {
        std::lock_guard<std::mutex> guard{callbacksMutex_};
        callbacks_.clear();
    }

    Network()
    {
        ADSTLOG_INIT_ACTOR_TRACE_MODULES("Network");
    }

    Network(const Network&) = delete;
    Network(Network&&)      = delete;

    Network& operator=(Network&&) = delete;
    Network& operator=(const Network&) = delete;

    /**
     * Registers a callback to an Event.
     *
     * @tparam Event The event type to be dispatched.
     * @param callback The callback which is scheduled for dispatching when event is published.
     * @return Unique handle (can be used later for unlisten)
     */
    template <typename Event>
    int listen(std::function<void(const std::shared_ptr<Event>)> callback)
    {
        static_assert(impl::validateEvent<Event>(), "Invalid event");

        const int handle = newHandle();
        listen<Event>(handle, std::move(callback));
        return handle;
    }

    /**
     * Broadcasts all event to all registered callbacks (from listen(..)).
     *
     * Callbacks are called sequentially.
     *
     * @tparam Event the event type to publish.
     * @param event Unique ptr to the event. Note: Event ownership is transferred to the network.
     */
    template <typename Event>
    void publish(std::unique_ptr<Event> event)
    {
        static_assert(impl::validateEvent<Event>(), "Invalid event");
        std::shared_ptr<Event>      shareableEvent = std::move(event);
        std::lock_guard<std::mutex> lambdaCallbacksGuard{callbacksMutex_};

        using Vector = impl::AsyncCallbackVector<std::shared_ptr<Event>>;
        auto found   = callbacks_.find(impl::type_id<Event>());
        LOG_C_D("%s", impl::getTypeName<Event>().c_str());
        if (found == callbacks_.end()) {
            LOG_C_W("no listener:{%s}", impl::getTypeName<Event>().c_str());
            return; // no such notifications
        }

        std::unique_ptr<impl::CallbackVector>& vector = found->second;
        //            assert(dynamic_cast<Vector*>(vector.get()));
        auto callbacks = static_cast<Vector*>(vector.get());
        LOG_C_T("listeners:{%s}, size=%d", impl::getTypeName<Event>().c_str(), callbacks->container_.size());
        for (const auto& element : callbacks->container_) {
            element.second(shareableEvent);
        }
    }

private:
    /**
     * Stores the AsyncCallBackVector by base class CallbackVector event is hidden.
     */
    using CallBackVector = std::unique_ptr<impl::CallbackVector>;
    /**
     * Mapping of type_id and CallBackVector is used in runtime to get back the callbacks for a specific event type.
     */
    using TypeIdToCallBackVector = std::map<impl::type_id_t, CallBackVector>;

    /**
     * Registers or Overwrites a Callback in the internal store with the given callback function
     *
     * @tparam Event The type of the event the callback takes as shared pointer
     * @param handle The unique handle assigned by the newHandle() function
     * @param callback The callback to register
     */
    template <typename Event>
    void listen(const CallBackHandle handle, std::function<void(const std::shared_ptr<Event>)> callback)
    {
        std::lock_guard<std::mutex> lambdaCallbacksGuard{callbacksMutex_};

        using Vector = impl::AsyncCallbackVector<std::shared_ptr<Event>>;
        // GCOVR_EXCL_START
        assert(callback && "callback should be valid"); // Check for valid object
        // GCOVR_EXCL_STOP
        std::unique_ptr<impl::CallbackVector>& vector = callbacks_[impl::type_id<Event>()];
        // GCOVR_EXCL_START
        if (vector == nullptr) {
            // GCOVR_EXCL_STOP
            vector.reset(new Vector{});
        }
        //            assert(dynamic_cast<Vector*>(vector.get())); // ToDo RSZRSZ Create ifdef for RTTI enabled ...
        auto callbacks = static_cast<Vector*>(vector.get());
        callbacks->add(handle, callback);
    }
    /**
     * Creates a unique handle used for callback listen.
     *
     * @return Unique handle used in listen to identify a callback.
     */
    CallBackHandle newHandle()
    {
        std::lock_guard<std::mutex> guard{callbacksMutex_};
        auto                        handle = ++handleCounter_;
        return handle;
    }

    CallBackHandle         handleCounter_ = 0;  /// counter used as for unique handle generation
    TypeIdToCallBackVector callbacks_     = {}; /// lookup of type to callback function
    mutable std::mutex     callbacksMutex_;     /// the guard mutex for multi threaded access
    ADSTLOG_DEF_ACTOR_TRACE_MODULES();
};

} // namespace adst::ep::test_engine::core
