#pragma once

#include "common.hpp"
#include "core/core_types.hpp"

namespace adst::ep::test_engine::core::impl {

/**
 * Interface class for the callback type store.
 * Note: Even though it is called vector, the underlying implementation may differ from std:vector.
 */
struct CallbackVector
{
    virtual ~CallbackVector() = default;

    /**
     * Removes the callback identified by handle.
     * @param handle Unique handle (created by listen function, see Port::listen(..))
     * @return True in case the Vector becomes empty, false otherwise.
     */
    virtual bool remove(const CallBackHandle handle) = 0;
};

} // namespace adst::ep::test_engine::core::impl
