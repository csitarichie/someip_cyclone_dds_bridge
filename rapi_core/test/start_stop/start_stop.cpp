#include <istream>
#include "core/priority.hpp"
#include "gtest/gtest.h"
#include "test_helper/border_actor_comps.hpp"
#include "test_helper/common_helper.hpp"
#include "test_helper/simple_actors.hpp"

using adst::common::Config;
using adst::common::Error;
using adst::ep::test_engine::core::Actor;
using adst::ep::test_engine::core::Environment;
using adst::ep::test_engine::core::StartCnf;
using adst::ep::test_engine::core::StartReq;
using adst::ep::test_engine::core::StopCnf;

namespace test_helper = adst::ep::test_engine::test_helper;

std::atomic_int32_t leafStartCnfHandle_count = 0;
std::atomic_int32_t leadStopCnfHandle_count  = 0;

std::atomic_int32_t middleStartReqHandle_count = 0;
std::atomic_int32_t middleStartCnfHandle_count = 0;

std::atomic_int32_t middleLeaf4StartCnfHandle_count = 0;

std::atomic_int32_t rootStopCnfHandle_count = 0;

std::atomic_int32_t startCnfLeaf5_count = 0;
std::atomic_int32_t stopCnfLeaf5_count  = 0;

static constexpr int EXPECTED_ERROR_CODE = 42;

// tidy doesn't like basic EXPECT_EQ
#define EXPECT_EQ_NOLINT(a, b) EXPECT_EQ(a, b) // NOLINT

template <int number>
struct LeafNode : public Actor
{
    // not copyable or movable
    LeafNode(const LeafNode&) = delete;
    LeafNode(LeafNode&&)      = delete;

    LeafNode& operator=(const LeafNode&) = delete;
    LeafNode& operator=(LeafNode&&) = delete;

    using StartLeafCnf = StartCnf<LeafNode<number>>;

    using StopLeafCnf = StopCnf<LeafNode<number>>;

    explicit LeafNode(const Environment& env)
        : Actor(NAME.c_str(), env)
    {
        listen<StartLeafCnf>([this](const StartLeafCnf& startCnf) { handleStartCnf(startCnf); });
    }
    ~LeafNode() override = default;

    void handleStartCnf(const StartLeafCnf& /*unused*/)
    {
        LOG_I("handling user '%s'", StartLeafCnf::NAME.c_str());
        listen<StopLeafCnf>([this](const StopLeafCnf&) {
            LOG_I("handling user '%s'", StopLeafCnf::NAME.c_str());
            leadStopCnfHandle_count++;
        });
        leafStartCnfHandle_count++;
    }

    static constexpr const auto NAME = sstr::literal("LeafNode") + test_helper::NumToChar<number>::VALUE;
};

template <int num>
struct MiddleNode : public Actor
{
    // not copyable or movable
    MiddleNode(const MiddleNode&) = delete;
    MiddleNode(MiddleNode&&)      = delete;

    MiddleNode& operator=(const MiddleNode&) = delete;
    MiddleNode& operator=(MiddleNode&&) = delete;

    using StartMidReq  = StartReq<MiddleNode<num>>;
    using StopCnfLeaf4 = StopCnf<LeafNode<4>>;
    using StartMidCnf  = StartCnf<MiddleNode<num>>;

    explicit MiddleNode(const Environment& env)
        : Actor(NAME.c_str(), env)
    {
        newChild<LeafNode<num * 3 + 1>>();
        newChild<LeafNode<num * 3 + 2>>();
        newChild<LeafNode<num * 3 + 3>>();
        listen<StartMidReq>([this](const StartMidReq&) {
            LOG_I("handling '%s'", StartMidReq::NAME.c_str());
            middleStartReqHandle_count++;

            listen<StopCnfLeaf4>([this](const StopCnfLeaf4&) {
                LOG_I("handling '%s'", StopCnfLeaf4::NAME.c_str());
                middleLeaf4StartCnfHandle_count++;
            });

            listen<StartMidCnf>([this](const StartMidCnf&) {
                LOG_I("handling '%s'", StartMidCnf::NAME.c_str());
                middleStartCnfHandle_count++;
            });
        });
    }
    ~MiddleNode() override = default;

    static constexpr const auto NAME = sstr::literal("MiddleNode") + test_helper::NumToChar<num>::VALUE;
};

struct StartStopRoot : public Actor
{
    // not copyable or movable
    StartStopRoot(const StartStopRoot&) = delete;
    StartStopRoot(StartStopRoot&&)      = delete;

    StartStopRoot& operator=(const StartStopRoot&) = delete;
    StartStopRoot& operator=(StartStopRoot&&) = delete;

    explicit StartStopRoot(const Environment& env)
        : Actor(NAME.c_str(), env)
    {
        newChild<MiddleNode<1>>();
        newChild<MiddleNode<2>>();

        using StartRootCnf = StartCnf<StartStopRoot>;
        listen<StartRootCnf>([this](const StartRootCnf&) { publish<Stop>(std::make_unique<Stop>()); });

        using StopRootCnf = StopCnf<StartStopRoot>;
        listen<StopRootCnf>([](const StopRootCnf&) { rootStopCnfHandle_count++; });

        using SelfStartReq  = StartReq<StartStopRoot>;
        using StartCnfLeaf5 = StartCnf<LeafNode<5>>;
        listen<SelfStartReq>([this](const SelfStartReq&) {
            listen<StartCnfLeaf5>([](const StartCnfLeaf5&) { startCnfLeaf5_count++; });
        });

        using StopCnfLeaf5 = StopCnf<LeafNode<5>>;
        listen<StopCnfLeaf5>([](const StopCnfLeaf5&) { stopCnfLeaf5_count++; });
    }
    ~StartStopRoot() override        = default;
    static constexpr const auto NAME = sstr::literal("StartStopRoot");
};

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(EPTestEngineCoreTest, StartStopBasic)
{
    test_helper::runCoreDefault<StartStopRoot>();
    EXPECT_EQ_NOLINT(leafStartCnfHandle_count, 6);
    EXPECT_EQ_NOLINT(leadStopCnfHandle_count, 6);
    EXPECT_EQ_NOLINT(middleStartReqHandle_count, 2);
    EXPECT_EQ_NOLINT(middleStartCnfHandle_count, 2);
    EXPECT_EQ_NOLINT(middleLeaf4StartCnfHandle_count, 2);
    EXPECT_EQ_NOLINT(rootStopCnfHandle_count, 1);
    EXPECT_EQ_NOLINT(startCnfLeaf5_count, 1);
    EXPECT_EQ_NOLINT(stopCnfLeaf5_count, 1);
}

struct StartStopDeadRoot : public Actor
{
    // not copyable or movable
    StartStopDeadRoot(const StartStopDeadRoot&) = delete;
    StartStopDeadRoot(StartStopDeadRoot&&)      = delete;

    StartStopDeadRoot& operator=(const StartStopDeadRoot&) = delete;
    StartStopDeadRoot& operator=(StartStopDeadRoot&&) = delete;

    explicit StartStopDeadRoot(const Environment& env)
        : Actor(NAME.c_str(), env)
    {
        // publishing too early
        publish<Stop>(std::make_unique<Stop>());
    }
    ~StartStopDeadRoot() override    = default;
    static constexpr const auto NAME = sstr::literal("StartStopDeadRoot");
};

// clang-format off
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, cert-err58-cpp, cppcoreguidelines-owning-memory, google-readability-function-size, readability-function-size)
TEST(EPTestEngineCoreTest, StartStopNoStart)
// clang-format on
{
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    auto test                               = [] {
        ConfigAndLogger confLog = {{[](const Error& error) {
                                        std::cerr << error.errorCodeName_.second << std::endl;
                                        exit(EXPECTED_ERROR_CODE);
                                    },
                                    Config::DEFAULT_NUMBER_OF_DISPATCHERS}};
        auto            core    = Core(confLog);
        core.init<StartStopDeadRoot>();
    };

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto, cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXPECTED_ERROR_CODE), "");
}
