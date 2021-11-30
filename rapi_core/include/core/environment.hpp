#pragma once

#include <core/network.hpp>
#include <iostream>
#include "adstutil_cxx/error_handler.hpp"
#include "config_cxx/config_and_logger.hpp"
#include "core/priority.hpp"

using adst::common::ConfigAndLogger;
using adst::common::Logger;

namespace adst::ep::test_engine::core {

class Core;
class Actor;

/**
 * A shared single read-only instance of an environment used by the core. It is passed to all actors.
 */
class Environment
{
public:
    explicit Environment(const ConfigAndLogger& configAndLogger);

    // not copyable or movable
    Environment(const Environment&) = delete;
    Environment(Environment&&)      = delete;

    Environment& operator=(const Environment&) = delete;
    Environment& operator=(Environment&&) = delete;

    Environment() = delete;
    /**
     * making config a part of the global environment to be access by all actors for
     * configuration data
     */
    const ConfigAndLogger& configAndLogger_;

#ifdef ADST_CORE_TEST_ENABLED
    /**
     * Only required by (and used for) tests.
     * @return
     */
    Priority& getPrioForTest() const
    {
        return priority_;
    }

    /**
     * Only required by (and used for) tests.
     * @return
     */
    Network& getNetworkForTest() const
    {
        return network_;
    }
#endif

    ~Environment() = default;

private:
    friend Core;
    friend Actor;
    /**
     * For now a single level of priority defined for ADST core all actors are executed in this priority level.
     */
    mutable Priority priority_ =
        Priority(configAndLogger_.config_.numberOfDispatchers_, configAndLogger_.config_.onErrorCallBack_);
    mutable Network network_ = Network{}; /// The only single network used by all Actors.
};

} // namespace adst::ep::test_engine::core
