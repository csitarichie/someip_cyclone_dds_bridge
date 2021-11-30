#include "gtest/gtest.h"

#include "core/core.hpp"
#include "sml_actor/sml_actor.hpp"

using adst::common::Config;
using adst::common::Error;
using adst::ep::test_engine::core::Actor;
using adst::ep::test_engine::core::Core;
using adst::ep::test_engine::core::Stop;
using adst::ep::test_engine::infra::SmlActor;
using adst::ep::test_engine::infra::SmlStartCnf;
using boost::sml::state;

const auto state1 = state<class State1>;

struct RootActorContext
{
    // not copyable but movable
    RootActorContext(const RootActorContext&) = delete;
    RootActorContext(RootActorContext&&)      = default;

    RootActorContext& operator=(const RootActorContext&) = delete;
    RootActorContext& operator=(RootActorContext&&) = default;

    RootActorContext()  = default;
    ~RootActorContext() = default;

    int                   theAnswer_ = 42;
    static constexpr auto NAME       = sstr::literal("SmlRootActor");
};

struct SmlRootActor
{
    auto operator()() const
    {
        // NOLINTNEXTLINE(google-build-using-namespace)
        using namespace boost::sml;

        const auto action = [](RootActorContext& cntx, Actor& actor) {
            LOG_I_ON(actor, "The Ultimate Answer to Life, The Universe and Everything is...%d!", cntx.theAnswer_);
            actor.publish(std::make_unique<Stop>());
        };

        // clang-format off
        return make_transition_table(
            *state1 + event<SmlStartCnf> / action = X
        );

        // clang-format on
    }
}; // struct SmlRootActor

static constexpr auto DEBUG_LEVEL = sstr::literal(
    "---\n"
    "logging:\n"
    "  client:\n"
    "    Trc.Verb: 2\n");

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(StateMachineActor, BasicStateMachineActor)
{
    using TestRootActor = SmlActor<SmlRootActor, RootActorContext>;
    auto confAndLog     = ConfigAndLogger{{[](const Error& error) {
                                           std::cout << error.errorCodeName_.second << std::endl;
                                           GTEST_FAIL();
                                       },
                                       Config::DEFAULT_NUMBER_OF_DISPATCHERS,
                                       "",
                                       {DEBUG_LEVEL}}};

    auto core = Core(confAndLog);
    core.init<TestRootActor>(RootActorContext());
    core.run();
}
