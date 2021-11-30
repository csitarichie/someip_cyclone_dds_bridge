
#include <csignal>
#include <iostream>

#include "adstlog_cxx/adstlog.hpp"

#include <fmt/format.h>
#include <string>
#include "adstutil_cxx/make_array.hpp"
#include "config_cxx/config.hpp"

using adst::common::Config;
using adst::common::Logger;

static constexpr auto DEFAULT_SINK     = sstr::literal("Console");
static constexpr auto DEFAULT_FORMAT   = sstr::literal("%tf %tn %cn %mn [%lv] [%fn] %ms");
static constexpr auto DEFAULT_TRC_VERB = 0;
static constexpr auto DEFAULT_POOL     = 32768;

// first we do registration fast later when we have the value
// we overwrite the handler.
std::function<void(int)> signalHandler = [](int sigNum) {
    fmt::print("\nadstlog:[{}] caught signal (SIGNUM={})\n", Config::LOGGER_NAME.c_str(), sigNum);

    P7_Exceptional_Flush();
};

void sigHandlerFunc(int sigNum)
{
    signalHandler(sigNum);

    // use convention that exit code due to signal is 128 + SIGNUM
    exit(0x80 + sigNum);
}

Logger::Logger(const Config& config)
{
    P7_Set_Crash_Handler();

    signal(SIGINT, sigHandlerFunc);
    signal(SIGTERM, sigHandlerFunc);
    signal(SIGQUIT, sigHandlerFunc);
    signal(SIGKILL, sigHandlerFunc);

    auto configLoggingName = config.configDoc_.getValue<std::string>("logging/name");

    auto loggerName = (configLoggingName.first) ? configLoggingName.second.c_str() : Config::LOGGER_NAME.c_str();

    // now handler with the proper name.
    signalHandler = [loggerName](int sigNum) {
        fmt::print("\nadstlog:[{}] caught signal (SIGNUM={})\n", loggerName, sigNum);

        P7_Exceptional_Flush();
    };

    // create P7 client object
    client_ = std::make_unique<ManageTraceObj<IP7_Client>>(&P7_Create_Client, getClientConfigStr(config).c_str(),
                                                           &IP7_Client::Release);

    // GCOVR_EXCL_START
    if (nullptr == client_->getObj()) {
        config.onErrorCallBack_(
            {{7, fmt::format("initializeLogging failed to create client: '{}'", loggerName).c_str()}});
        return;
    }
    // GCOVR_EXCL_STOP

    client_->getObj()->Share(loggerName);

    for (auto& channel : make_array(ADSTLOG_TRACE_CHANNELS)) {
        channels_.push_back(std::make_unique<ManageTraceObj<IP7_Trace>>(&P7_Create_Trace, client_->getObj(), channel,
                                                                        &IP7_Trace::Release));
        // GCOVR_EXCL_START
        if (nullptr == channels_.back()->getObj()) {
            config.onErrorCallBack_(
                {{7, fmt::format("initializeLogging failed to create channel: '{}'", channel).c_str()}});
            return;
        }
        // GCOVR_EXCL_STOP
        channels_.back()->getObj()->Share(channel);
    }
}

std::string Logger::getClientConfigStr(const adst::common::Config& config)
{
    auto configSink    = config.configDoc_.getValue<std::string>("/logging/client/Sink");
    auto configFormat  = config.configDoc_.getValue<std::string>("/logging/client/Format");
    auto configTrcVerb = config.configDoc_.getValue<int>("/logging/client/Trc.Verb");
    auto configPool    = config.configDoc_.getValue<int>("/logging/client/Pool");

    auto clientConfig = fmt::format(
        "/P7.Sink={}"
        " /P7.Format=\"{}\""
        " /P7.Trc.Verb={}"
        " /P7.Pool={}",
        (configSink.first) ? configSink.second : std::string{DEFAULT_SINK},
        (configFormat.first) ? configFormat.second : std::string{DEFAULT_FORMAT},
        (configTrcVerb.first) ? configTrcVerb.second : DEFAULT_TRC_VERB,
        (configPool.first) ? configPool.second : DEFAULT_POOL);

    std::cout << "console P7 trace config: '" << clientConfig << "'" << std::endl;
    return clientConfig;
}
