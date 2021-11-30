#pragma once

#include "adstlog_cxx/adstlog.hpp"
#include "config_cxx/config.hpp"

namespace adst::common {

struct ConfigAndLogger
{
    const Config config_ = {};
    Logger       logger_ = {config_};
};

} // namespace adst::common
