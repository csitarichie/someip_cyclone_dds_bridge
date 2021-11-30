#include <istream>
#include "core/core.hpp"
#include "gtest/gtest.h"

#include "adstutil_cxx/error_handler.hpp"
#include "adstutil_cxx/static_string.hpp"
#include "test_helper/common_helper.hpp"
#include "test_helper/simple_actors.hpp"

using adst::common::ConfigAndLogger;
using adst::common::Error;
using adst::ep::test_engine::core::StartCnf;

namespace test_helper = adst::ep::test_engine::test_helper;
namespace sstr        = ak_toolkit::static_str;

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(EPTestEngineCoreTest, Basic)
{
    test_helper::runCoreDefault<test_helper::RootActor>();
}

struct Ping final
{
    Ping() = default;
    explicit Ping(const int count)
        : count_(count)
    {
    }
    int                   count_ = 0;
    static constexpr auto NAME   = sstr::literal("Ping");
};

struct Pong final
{
    explicit Pong(int count)
        : count_(count)
    {
    }

    int                         count_ = 0;
    static constexpr const auto NAME   = sstr::literal("Pong");
};

struct Responder : public Actor
{
    // not copyable or movable
    Responder(const Responder&) = delete;
    Responder(Responder&&)      = delete;

    Responder& operator=(const Responder&) = delete;
    Responder& operator=(Responder&&) = delete;

    explicit Responder(const Environment& env)
        : Actor(NAME.c_str(), env)
    {
        LOG_I("ctor start");
        listen<Ping>([this](const Ping& msg) {
            int count = msg.count_;
            count++;
            publish(std::make_unique<Pong>(count));
        });
        LOG_I("ctor finish");
    }
    ~Responder() override = default;

    static constexpr const auto NAME = sstr::literal("Responder");
};

struct PingPongRoot : public Actor
{
    // not copyable or movable
    PingPongRoot(const PingPongRoot&) = delete;
    PingPongRoot(PingPongRoot&&)      = delete;

    PingPongRoot& operator=(const PingPongRoot&) = delete;
    PingPongRoot& operator=(PingPongRoot&&) = delete;

    using SelfStartCnf = StartCnf<PingPongRoot>;

    explicit PingPongRoot(const Environment& env)
        : Actor(NAME.c_str(), env)
    {
        LOG_I("ctor start");
        listen<SelfStartCnf>([this](const SelfStartCnf&) { publish<Ping>(std::make_unique<Ping>()); });

        listen<Pong>([this](const Pong& msg) {
            if (msg.count_ == 50000) {
                publish<Stop>(std::make_unique<Stop>());
            } else {
                publish(std::make_unique<Ping>(msg.count_));
            }
        });
        newChild<Responder>();
        LOG_I("ctor finish");
    }
    ~PingPongRoot() override = default;

    static constexpr const auto NAME = sstr::literal("PingPongRoot");
};

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(EPTestEngineCoreTest, PingPong)
{
    test_helper::runCoreDefault<PingPongRoot>();
}

static constexpr const int MESSAGE_COUNT = 10000;

template <int number>
struct Pings final
{
    Pings() = default;
    explicit Pings(const int count)
        : count_(count)
    {
    }
    int                         count_ = 0;
    static constexpr const int  inst   = number;
    static constexpr const auto NAME   = sstr::literal("Ping") + test_helper::NumToChar<number>::VALUE;
};

template <int number>
struct Pongs final
{
    explicit Pongs(int count)
        : count_(count)
    {
    }

    int                         count_ = 0;
    static constexpr const int  inst   = number;
    static constexpr const auto NAME   = sstr::literal("Pong") + test_helper::NumToChar<number>::VALUE;
};

template <int number>
struct Finish final
{
    static constexpr const int  inst = number;
    static constexpr const auto NAME = sstr::literal("Finish") + test_helper::NumToChar<number>::VALUE;
};

template <int number>
struct Responders : public Actor
{
    // not copyable or movable
    Responders(const Responders&) = delete;
    Responders(Responders&&)      = delete;

    Responders& operator=(const Responders&) = delete;
    Responders& operator=(Responders&&) = delete;

    using PingT = Pings<number>;
    using PongT = Pongs<number>;
    explicit Responders(const Environment& env)
        : Actor(NAME.c_str(), env)
    {
        LOG_I("ctor start");
        listen<PingT>([this](const PingT& msg) {
            int count = msg.count_;
            count++;
            publish(std::make_unique<PongT>(count));
        });
        LOG_I("listen:{%s}", PingT::NAME.c_str());
    }

    static constexpr const int inst = number;
    ~Responders() override          = default;

    static constexpr const auto NAME = sstr::literal("Responders") + test_helper::NumToChar<number>::VALUE;
};

template <int number>
struct Receivers : public Actor
{
    // not copyable or movable
    Receivers(const Receivers&) = delete;
    Receivers(Receivers&&)      = delete;

    Receivers& operator=(const Receivers&) = delete;
    Receivers& operator=(Receivers&&) = delete;

    using PingT             = Pings<number>;
    using PongT             = Pongs<number>;
    using FinishT           = Finish<number>;
    using ResponderStartCnf = StartCnf<Responders<number>>;
    using SelfStartCnf      = StartCnf<Receivers<number>>;
    explicit Receivers(const Environment& env)
        : Actor(NAME.c_str(), env)
    {
        LOG_I("ctor start");

        listen<ResponderStartCnf>([this](const ResponderStartCnf&) {
            LOG_D("state = %d", startupState_);
            startupState_ |= StartupState::RESPONDER_READY;
            LOG_D("state changed to %d", startupState_);
            sendPing();
        });

        listen<SelfStartCnf>([this](const SelfStartCnf&) {
            LOG_D("state = %d", startupState_);
            startupState_ |= StartupState::SELF_READY;
            LOG_D("state changed to %d", startupState_);
            sendPing();
        });

        listen<PongT>([this](const PongT& msg) {
            if (msg.count_ == MESSAGE_COUNT) {
                publish<FinishT>(std::make_unique<FinishT>());
            } else {
                publish(std::make_unique<PingT>(msg.count_));
            }
        });
        LOG_I("listen:{%s}", PongT::NAME.c_str());
    }

    void sendPing()
    {
        LOG_D("state=%d", startupState_);
        if (startupState_ == (StartupState::RESPONDER_READY | StartupState::SELF_READY)) {
            publish<PingT>(std::make_unique<PingT>());
        }
    }

    enum StartupState
    {
        NOTHING_READY   = 0,
        SELF_READY      = 1u << 0u,
        RESPONDER_READY = 1u << 1u
    };

    unsigned int startupState_ = StartupState::NOTHING_READY;

    static constexpr const int inst = number;
    ~Receivers() override           = default;

    static constexpr const auto NAME = sstr::literal("Receivers") + test_helper::NumToChar<number>::VALUE;
};

struct PingPongsRoot : public Actor
{
    // not copyable or movable
    PingPongsRoot(const PingPongsRoot&) = delete;
    PingPongsRoot(PingPongsRoot&&)      = delete;

    PingPongsRoot& operator=(const PingPongsRoot&) = delete;
    PingPongsRoot& operator=(PingPongsRoot&&) = delete;

    explicit PingPongsRoot(const Environment& env)
        : Actor(NAME.c_str(), env)
    {
        LOG_I("ctor start");
        createPingPongSet<test_helper::DISPATCHER_COUNT>();
        LOG_I("added children");
    }

    template <int number>
    void createPingPongSet()
    {
        createSinglePingPong<number - 1>();
        createPingPongSet<number - 1>();
    }

    template <int number>
    void createSinglePingPong()
    {
        listen<Finish<number>>([this](const Finish<number>&) { finished(); });
        newChild<Responders<number>>();
        newChild<Receivers<number>>();
    }

    void finished()
    {
        receivedFinishes_++;
        LOG_I("receivedFinishes_[%d]", receivedFinishes_);
        if (receivedFinishes_ == test_helper::DISPATCHER_COUNT) {
            publish<Stop>(std::make_unique<Stop>());
        }
    }
    int receivedFinishes_ = 0;

    ~PingPongsRoot() override = default;

    static constexpr const auto NAME = sstr::literal("PingPongsRoot");
};

template <>
void PingPongsRoot::createPingPongSet<0>()
{
}

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(EPTestEngineCoreTest, ParallelPingPong)
{
    test_helper::runCoreDefault<PingPongsRoot>();
}

struct StandaloneActor : public Actor
{
    // not copyable or movable
    StandaloneActor(const StandaloneActor&) = delete;
    StandaloneActor(StandaloneActor&&)      = delete;

    StandaloneActor& operator=(const StandaloneActor&) = delete;
    StandaloneActor& operator=(StandaloneActor&&) = delete;

    explicit StandaloneActor(const Environment& env)
        : Actor(NAME.c_str(), env)
    {
        ctorFinished();
    }
    ~StandaloneActor() override      = default;
    static constexpr const auto NAME = sstr::literal("StandaloneActor");
};

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(EPTestEngineCoreTest, StandaloneActor)
{
    ConfigAndLogger confLog = {{[](const Error& error) {
                                    std::cout << error.errorCodeName_.second << std::endl;
                                    GTEST_FAIL();
                                    exit(1);
                                },
                                test_helper::DISPATCHER_COUNT}};

    Environment     env(confLog);
    StandaloneActor act(env);
}
