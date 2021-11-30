#include <istream>

#include "adstutil_cxx/error_handler.hpp"

#include "core/priority.hpp"
#include "gtest/gtest.h"
#include "test_helper/simple_actors.hpp"

using adst::common::Config;
using adst::common::ConfigAndLogger;
using adst::common::Error;
using adst::ep::test_engine::core::Actor;
using adst::ep::test_engine::core::Environment;

static constexpr int EXPECTED_ERROR_CODE = 42;

static constexpr int DISPATCHER_COUNT = 8;

// NOLINTNEXTLINE(cppcoreguidelines-interfaces-global-init)
auto onError = [](const Error& error) {
    std::cout << error.errorCodeName_.second << std::endl;
    GTEST_FAIL();
    exit(1);
};

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(EPTestEngineCoreTest, PriorityBasic)
{
    ConfigAndLogger confLog = {Config{onError, DISPATCHER_COUNT}};
    auto            env     = Environment(confLog);
    auto&           prio{env.getPrioForTest()};

    struct ScheduleAble
    {
        void printName()
        {
            std::cout << name_ << std::endl;
        }

        std::string name_ = "ScheduleAble";
    };

    ScheduleAble first{"First"}, second{"Second"}, third{"Third"};

    prio.start();
    prio.schedule([&first] { first.printName(); });
    prio.schedule([&second] { second.printName(); });
    prio.schedule([&third] { third.printName(); });
    //    sleep(2);

    for (int i = 0; i < 100; ++i) {
        prio.schedule([&first] { first.printName(); });
        prio.schedule([&second] { second.printName(); });
        prio.schedule([&third] { third.printName(); });
    }

    //    sleep(10);
    prio.schedule([&first] { first.printName(); });
    prio.schedule([&second] { second.printName(); });
    prio.schedule([&third] { third.printName(); });
    prio.stop();
}

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(EPTestEngineCoreTest, PriorityConstructorDestructor)
{
    ConfigAndLogger confLog = {Config{onError, DISPATCHER_COUNT}};
    auto            env     = Environment(confLog);
}

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(EPTestEngineCoreTest, PriorityDelayedStart)
{
    auto onErrorExpected = [](const Error& error) {
        std::cout << error.errorCodeName_.second << std::endl;
        EXPECT_STREQ(error.errorCodeName_.second.c_str(), "Priority:Stop() Called without start.");
        GTEST_SUCCEED();
        exit(0);
    };
    ConfigAndLogger confLog = {Config{onErrorExpected, DISPATCHER_COUNT}};
    auto            env     = Environment(confLog);
    auto&           prio    = env.getPrioForTest();
    sleep(1);
    prio.start();
}

// clang-format off
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, cert-err58-cpp, cppcoreguidelines-owning-memory, google-readability-function-size, readability-function-size)
TEST(EPTestEngineCoreTest, PriorityStopWithoutStart)
// clang-format on
{
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    auto test                               = [] {
        auto onErrorExpected = [](const Error& error) {
            std::cerr << error.errorCodeName_.second << std::flush;
            EXPECT_STREQ(error.errorCodeName_.second.c_str(), "Priority:Stop() Called without start.");
            exit(EXPECTED_ERROR_CODE);
        };
        ConfigAndLogger confLog = {Config{onErrorExpected, DISPATCHER_COUNT}};
        auto            env     = Environment(confLog);
        auto&           prio    = env.getPrioForTest();
        prio.stop();
    };
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto, cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXPECTED_ERROR_CODE), "");
}

// clang-format off
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, cert-err58-cpp, cppcoreguidelines-owning-memory, google-readability-function-size, readability-function-size)
TEST(EPTestEngineCoreTest, PriorityTowStart)
// clang-format on
{
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    auto onErrorExpected                    = [](const Error& error) {
        std::cout << error.errorCodeName_.second << std::endl;
        EXPECT_STREQ(error.errorCodeName_.second.c_str(), "Priority:Start() Can be called only ones.");
        exit(EXPECTED_ERROR_CODE);
    };
    ConfigAndLogger confLog = {Config{onErrorExpected, DISPATCHER_COUNT}};
    auto            env     = Environment(confLog);
    auto&           prio    = env.getPrioForTest();
    prio.start();
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto, cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    EXPECT_EXIT(prio.start();, ::testing::ExitedWithCode(EXPECTED_ERROR_CODE), "");
}

struct PortTestRoot : public Actor
{
    // not copiable or movable
    PortTestRoot(const PortTestRoot&) = delete;
    PortTestRoot& operator=(const PortTestRoot&) = delete;
    PortTestRoot(PortTestRoot&&)                 = delete;
    PortTestRoot& operator=(PortTestRoot&&) = delete;

    explicit PortTestRoot(const Environment* env)
        : Actor(NAME.c_str(), *env)
        , envPrt_(env)
    {
    }

    ~PortTestRoot() override
    {
        delete envPrt_;
    }
    const Environment*          envPrt_ = nullptr;
    static constexpr const auto NAME    = sstr::literal("PortTestRoot");
};

// clang-format off
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, cert-err58-cpp, cppcoreguidelines-owning-memory, google-readability-function-size, readability-function-size)
TEST(EPTestEngineCoreTest, ScheduleNoListen)
// clang-format on
{
    using StartReq       = StartReq<PortTestRoot>;
    auto onErrorExpected = [](const Error& error) {
        std::cout << error.errorCodeName_.second << std::endl;
        EXPECT_STREQ(error.errorCodeName_.second.c_str(), "Priority:Start() Can be called only ones.");
        exit(EXPECTED_ERROR_CODE);
    };
    ConfigAndLogger confLog = {Config{onErrorExpected, DISPATCHER_COUNT}};
    // this is ugly but for test is OK I think :). there is the Deal that the root can go out of scope
    // when the environment is destructed. and this was the easiest to achieve it.
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    auto*        env = new Environment(confLog);
    PortTestRoot root(env);
    auto&        port = root.getPortForTest(0);
    port.schedule(std::make_unique<StartReq>());
}

// clang-format off
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, cert-err58-cpp, cppcoreguidelines-owning-memory, google-readability-function-size, readability-function-size)
TEST(EPTestEngineCoreTest, Schedule3Start)
// clang-format on
{
    using StartReq       = StartReq<PortTestRoot>;
    auto onErrorExpected = [](const Error& error) {
        std::cout << error.errorCodeName_.second << std::endl;
        EXPECT_STREQ(error.errorCodeName_.second.c_str(), "Priority:Start() Can be called only ones.");
        exit(EXPECTED_ERROR_CODE);
    };
    ConfigAndLogger confLog = {Config{onErrorExpected, DISPATCHER_COUNT}};
    // this is ugly but for test is OK I think :). there is the Deal that the root can go out of scope
    // when the environment is destructed. and this was the easiest to achieve it.
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    auto*        env = new Environment(confLog);
    PortTestRoot root(env);
    auto&        port = root.getPortForTest(0);
    port.listen<StartReq>([](const StartReq&) { std::cout << "Start 1" << std::endl; });
    port.listen<StartReq>([](const StartReq&) { std::cout << "Start 2" << std::endl; });
    port.schedule(std::make_unique<StartReq>());
    port.schedule(std::make_unique<StartReq>());
    port.schedule(std::make_unique<StartReq>());
}

// clang-format off
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, cert-err58-cpp, cppcoreguidelines-owning-memory, google-readability-function-size, readability-function-size)
TEST(EPTestEngineCoreTest, ScheduleUnlistenAll)
// clang-format on
{
    using StartReq       = StartReq<PortTestRoot>;
    auto onErrorExpected = [](const Error& error) {
        std::cout << error.errorCodeName_.second << std::endl;
        EXPECT_STREQ(error.errorCodeName_.second.c_str(), "Priority:Start() Can be called only ones.");
        exit(EXPECTED_ERROR_CODE);
    };
    ConfigAndLogger confLog = {Config{onErrorExpected, DISPATCHER_COUNT}};
    // this is ugly but for test is OK I think :). there is the Deal that the root can go out of scope
    // when the environment is destructed. and this was the easiest to achieve it.
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    auto*        env = new Environment(confLog);
    PortTestRoot root(env);
    auto&        port = root.getPortForTest(0);

    auto handle = port.listen<StartReq>([](const StartReq&) { std::cout << "Start 1" << std::endl; });
    port.listen<StartReq>([](const StartReq&) { std::cout << "Start 2" << std::endl; });

    port.unlistenAll(handle);
    port.schedule(std::make_unique<StartReq>());
}

// clang-format off
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, cert-err58-cpp, cppcoreguidelines-owning-memory, google-readability-function-size, readability-function-size)
TEST(EPTestEngineCoreTest, ScheduleUnlisten)
// clang-format on
{
    using StartReq       = StartReq<PortTestRoot>;
    auto onErrorExpected = [](const Error& error) {
        std::cout << error.errorCodeName_.second << std::endl;
        EXPECT_STREQ(error.errorCodeName_.second.c_str(), "Priority:Start() Can be called only ones.");
        exit(EXPECTED_ERROR_CODE);
    };
    ConfigAndLogger confLog = {Config{onErrorExpected, DISPATCHER_COUNT}};
    // this is ugly but for test is OK I think :). there is the Deal that the root can go out of scope
    // when the environment is destructed. and this was the easiest to achieve it.
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    auto*        env = new Environment(confLog);
    PortTestRoot root(env);
    auto&        port   = root.getPortForTest(0);
    auto         handle = port.listen<StartReq>([](const StartReq&) { std::cout << "Start 1" << std::endl; });
    port.unlisten<StartReq>(handle);
    port.schedule(std::make_unique<StartReq>());
}

// clang-format off
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, cert-err58-cpp, cppcoreguidelines-owning-memory, google-readability-function-size, readability-function-size)
TEST(EPTestEngineCoreTest, ScheduleUnlistenWrong)
// clang-format on
{
    using StartReq       = StartReq<PortTestRoot>;
    auto onErrorExpected = [](const Error& error) {
        std::cout << error.errorCodeName_.second << std::endl;
        EXPECT_STREQ(error.errorCodeName_.second.c_str(), "Priority:Start() Can be called only ones.");
        exit(EXPECTED_ERROR_CODE);
    };
    ConfigAndLogger confLog = {Config{onErrorExpected, DISPATCHER_COUNT}};
    // this is ugly but for test is OK I think :). there is the Deal that the root can go out of scope
    // when the environment is destructed. and this was the easiest to achieve it.
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    auto*        env = new Environment(confLog);
    PortTestRoot root(env);
    auto&        port   = root.getPortForTest(0);
    auto         handle = port.listen<StartReq>([](const StartReq&) { std::cout << "Start 1" << std::endl; });
    port.unlisten<Stop>(handle);
    port.unlisten<StartReq>(handle);
    port.schedule(std::make_unique<StartReq>());
}

// clang-format off
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, cert-err58-cpp, cppcoreguidelines-owning-memory, google-readability-function-size, readability-function-size)
TEST(EPTestEngineCoreTest, ScheduleNetworkNoListen)
// clang-format on
{
    auto onErrorExpected = [](const Error& error) {
        std::cout << error.errorCodeName_.second << std::endl;
        EXPECT_STREQ(error.errorCodeName_.second.c_str(), "Priority:Start() Can be called only ones.");
        exit(EXPECTED_ERROR_CODE);
    };
    ConfigAndLogger confLog = {Config{onErrorExpected, DISPATCHER_COUNT}};
    // this is ugly but for test is OK I think :). there is the Deal that the root can go out of scope
    // when the environment is destructed. and this was the easiest to achieve it.
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    auto*        env = new Environment(confLog);
    PortTestRoot root(env);
    env->getNetworkForTest().publish(std::make_unique<StartReq<PortTestRoot>>());
}
