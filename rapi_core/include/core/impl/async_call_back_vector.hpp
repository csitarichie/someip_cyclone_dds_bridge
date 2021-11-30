#pragma once

#include <algorithm>
#include <functional>
#include <vector>

#include "call_back_vector.hpp"

namespace adst::ep::test_engine::core::impl {

/**
 * Implements an iterable container for callbacks.
 * @tparam Event Common event type which is used in the callback as input parameter.
 */
template <typename Event>
struct AsyncCallbackVector : public CallbackVector
{
    using CallbackType = std::function<void(const Event&)>;      /// Prototype of the callback.
    std::unordered_map<CallBackHandle, CallbackType> container_; /// Each callback is uniquely identified by a handle.

    /**
     * Removes the callback function from the store.
     * @param handle The unique handle of the callback function assigned during listen.
     * @return True in case the store becomes empty, false otherwise.
     */
    virtual bool remove(const CallBackHandle handle) override
    {
        container_.erase(handle);
        return container_.empty();
    }

    /**
     * Adds or replaces a callback in the store.
     * @param handle Unique handle of the callback function (assigned during listen).
     * @param callback The callback to add (or to use as a new callback instead of an already existing one).
     */
    void add(const CallBackHandle handle, CallbackType callback)
    {
        container_[handle] = std::move(callback);
    }
};

} // namespace adst::ep::test_engine::core::impl
