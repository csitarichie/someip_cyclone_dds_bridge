#pragma once

#include "adstutil_cxx/error_handler.hpp"
#include "config_cxx/yaml_wrapper.hpp"

ADST_DISABLE_CLANG_WARNING("unused-parameter")
#include "adstutil_cxx/static_string.hpp"
ADST_RESTORE_CLANG_WARNING()

namespace sstr = ak_toolkit::static_str;

namespace adst::common {

/**
 * Holds all runtime data for configuring the Core
 * and the error callback.
 */
struct Config
{
    static constexpr int  DEFAULT_NUMBER_OF_DISPATCHERS = 4; /// default number of dispatchers.
    static constexpr auto LOGGER_NAME                   = sstr::literal("TEST_ENGINE");
    static constexpr auto CONFIG_FILE_NAME              = sstr::literal("config.yml");
    static constexpr auto DEFAULT_CONFIG                = sstr::literal(
        "---\n"
        "logging:\n"
        "  client:\n"
        "    Sink: Console\n"
        "    Format: '%tf %tn %cn %mn [%lv] [%fn] %ms'\n"
        "    Trc.Verb: 2\n"
        "    Pool: 32768\n");

    /// default implementation of error handler callback
    adst::common::OnErrorCallBack onErrorCallBack_ = ::adst::common::onErrorFunc;
    const int   numberOfDispatchers_ = DEFAULT_NUMBER_OF_DISPATCHERS; /// number of dispatchers created in a priority
    std::string defConfFileName_     = {CONFIG_FILE_NAME};
    std::string defConfFileContent_  = {DEFAULT_CONFIG};
    /// config key value pairs make sure that it is the last member so list initialises can omit it.
    YamlWrapper configDoc_ = YamlWrapper(onErrorCallBack_, defConfFileContent_, defConfFileName_);
};

} // namespace adst::common
