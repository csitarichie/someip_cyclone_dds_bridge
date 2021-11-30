#include "gtest/gtest.h"

#include "config_cxx/config_and_logger.hpp"

using adst::common::Config;
using adst::common::ConfigAndLogger;
using adst::common::Error;

struct LogUser
{
    LogUser()
    {
        ADSTLOG_INIT_ACTOR_TRACE_MODULES("Defaultinit");
    }

    void test()
    {
        LOG_D("Test Log Entry");
    }
    ADSTLOG_DEF_ACTOR_TRACE_MODULES();
};

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(CommonAdstLogCxx, DefaultInit)
{
    ConfigAndLogger confLog = {{[](const Error& error) {
                                    std::cout << error.errorCodeName_.second << std::endl;
                                    GTEST_FAIL();
                                },
                                Config::DEFAULT_NUMBER_OF_DISPATCHERS, ""}};

    LogUser logUser;
    logUser.test();
}

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(CommonAdstLogCxx, DefaultFileInit)
{
    ConfigAndLogger confLog = {{[](const Error& error) {
                                    std::cout << error.errorCodeName_.second << std::endl;
                                    GTEST_FAIL();
                                },
                                Config::DEFAULT_NUMBER_OF_DISPATCHERS}};

    LogUser logUser;
    logUser.test();
}
