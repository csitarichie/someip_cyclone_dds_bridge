#pragma once

#include <queue>
#include <type_traits>
#include "adstutil_cxx/compiler_diagnostics.hpp"
#include "core/actor.hpp"

ADST_DISABLE_GCC_WARNING("shadow")
ADST_DISABLE_GCC_WARNING("subobject-linkage")
#include "boost/sml.hpp"
ADST_RESTORE_GCC_WARNING()
ADST_RESTORE_GCC_WARNING()

using adst::ep::test_engine::core::Actor;
using adst::ep::test_engine::core::Environment;
using boost::sml::sm;

namespace adst::ep::test_engine::infra {

template <class SmlSmT, class ContextT>
class SmlActor;

/**
 * Logger class to log boost::sml transitions via a dedicated trace module
 * `<ACTOR_NAME>_SML`
 */
struct SmlActorLogger
{
    SmlActorLogger(const char* name)
    {
        ADSTLOG_INIT_ACTOR_TRACE_MODULES(fmt::format("{}.Sml", name).c_str());
    }

    template <class SM, class TEvent>
    // NOLINTNEXTLINE(readability-named-parameter)
    void log_process_event(const TEvent&)
    {
        LOG_D("RX:[%s]", boost::sml::aux::get_type_name<TEvent>());
    }

    template <class SM, class TGuard, class TEvent>
    // NOLINTNEXTLINE(readability-named-parameter)
    void log_guard(const TGuard&, const TEvent&, bool result)
    {
        LOG_D("E[%s] G:[%s] R:[%s]", boost::sml::aux::get_type_name<TEvent>(), boost::sml::aux::get_type_name<TGuard>(),
              result ? "OK" : "Reject");
    }

    template <class SM, class TAction, class TEvent>
    // NOLINTNEXTLINE(readability-named-parameter)
    void log_action(const TAction&, const TEvent&)
    {
        LOG_D("E[%s] A:[%s]", boost::sml::aux::get_type_name<TEvent>(), boost::sml::aux::get_type_name<TAction>());
    }

    template <class SM, class TSrcState, class TDstState>
    void log_state_change(const TSrcState& src, const TDstState& dst)
    {
        LOG_I("[%s] -> [%s]", src.c_str(), dst.c_str());
    }

    ADSTLOG_DEF_ACTOR_TRACE_MODULES();
}; // struct SmlCoutLogger

using adst::ep::test_engine::core::StartCnf;
using adst::ep::test_engine::core::StartReq;
using adst::ep::test_engine::core::StopCnf;
using adst::ep::test_engine::core::StopReq;

// Unfortunately it is not possible to use the StartReq from core directly in sml because the
// StartReq is templated with the sml state machine class itself. As a result the auto return for
// the function call operator needs the complete event type and that is a snake biting it's own
// tail.
//
// As a workaround own events for a sml state machine are declared here and the SmlActor converts them from
// core::StartReq/Cnf/StopReq/Cnf to SmlStartReq/Cnf, SmlStopReq/Cnf. see: forwardToSml() function.
// this can go away once we have multiple networks.
struct SmlStartReq
{
    // not copyable or movable
    SmlStartReq(const SmlStartReq&) = delete;
    SmlStartReq(SmlStartReq&&)      = delete;

    SmlStartReq& operator=(const SmlStartReq&) = delete;
    SmlStartReq& operator=(SmlStartReq&&) = delete;

    static constexpr auto NAME = sstr::literal("SmlStartReq");

private:
    template <class SmlSmT, class ContextT>
    friend class SmlActor;

    SmlStartReq() = default;
};

struct SmlStartCnf
{
    // not copyable or movable
    SmlStartCnf(const SmlStartCnf&) = delete;
    SmlStartCnf(SmlStartReq&&)      = delete;

    SmlStartCnf& operator=(const SmlStartCnf&) = delete;
    SmlStartCnf& operator=(SmlStartCnf&&) = delete;

    static constexpr auto NAME = sstr::literal("SmlStartCnf");

private:
    template <class SmlSmT, class ContextT>
    friend class SmlActor;

    SmlStartCnf() = default;
};

struct SmlStopReq
{
    // not copyable or movable
    SmlStopReq(const SmlStopReq&) = delete;
    SmlStopReq(SmlStopReq&&)      = delete;

    SmlStopReq& operator=(const SmlStopReq&) = delete;
    SmlStopReq& operator=(SmlStopReq&&) = delete;

    static constexpr auto NAME = sstr::literal("SmlStopReq");

private:
    template <class SmlSmT, class ContextT>
    friend class SmlActor;

    SmlStopReq() = default;
};

struct SmlStopCnf
{
    // not copyable or movable
    SmlStopCnf(const SmlStopCnf&) = delete;
    SmlStopCnf(SmlStopCnf&&)      = delete;

    SmlStopCnf& operator=(const SmlStopCnf&) = delete;
    SmlStopCnf& operator=(SmlStopCnf&&) = delete;

    static constexpr auto NAME = sstr::literal("SmlStopCnf");

private:
    template <class SmlSmT, class ContextT>
    friend class SmlActor;

    SmlStopCnf() = default;
};

/**
 * Base Class for the adst code to bind SML state machine transaction tables and context and Actor
 * together as a state machine actor.
 *
 * It registers listen for all event types in the SmlSmT transition table.
 *
 * @tparam SmlSmT The sml state machine transition table.
 * @tparam ContextT The user class used as context in the transition table (e.g. for storing user data).
 */
template <class SmlSmT, class ContextT>
class SmlActor : public Actor
{
public:
    using SelfStartReq = StartReq<SmlActor<SmlSmT, ContextT>>;
    using SelfStartCnf = StartCnf<SmlActor<SmlSmT, ContextT>>;
    using SelfStopReq  = StopReq<SmlActor<SmlSmT, ContextT>>;
    using SelfStopCnf  = StopCnf<SmlActor<SmlSmT, ContextT>>;

    using SmlImpSm = sm<SmlSmT, boost::sml::logger<SmlActorLogger>, boost::sml::defer_queue<std::deque>,
                        boost::sml::process_queue<std::queue>>;

    /**
     * Helper class used as visitor in boost::sml::type_list.
     *
     * @tparam boost::sml::type_list
     */
    template <typename...>
    struct Caller;

    /**
     * Creates the binding class between sml backend and Actor.

     * It instantiates a logger and stores a context provided by the user.
     *
     * @param env The Environment instance to use.
     * @param cntx ContextT class instance.
     */
    SmlActor(const Environment& env, ContextT cntx)
        : Actor(NAME.c_str(), env)
        , cntx_(std::move(cntx))
        , logger_(NAME.c_str())
        , smlSm_{logger_, cntx_, static_cast<Actor&>(*this)}
    {
        using ListenEvnets = boost::sml::aux::apply_t<Caller, typename SmlImpSm::events>;

        ListenEvnets::call(*this);
    }

    /**
     * Listens on events from SML transition table and calls process event.
     *
     * @tparam MessageT The message to listen to.
     */
    template <typename MessageT>
    void forwardToSml()
    {
        if constexpr (std::is_same<SmlStartReq, MessageT>::value) {
            listen<SelfStartReq>([this](const SelfStartReq&) { smlSm_.process_event(SmlStartReq()); });
        } else if (std::is_same<SmlStopReq, MessageT>::value) {
            listen<SelfStopReq>([this](const SelfStopReq&) { smlSm_.process_event(SmlStopReq()); });
        } else if (std::is_same<SmlStartCnf, MessageT>::value) {
            listen<SelfStartCnf>([this](const SelfStartCnf&) { smlSm_.process_event(SmlStartCnf()); });
        } else if (std::is_same<SmlStopCnf, MessageT>::value) {
            listen<SelfStopCnf>([this](const SelfStopCnf&) { smlSm_.process_event(SmlStopCnf()); });
        } else if (boost::sml::aux::is_same<boost::sml::initial, MessageT>::value) {
            return; // ToDo RSZRSZ debug this why it is not working ...
        } else {
            listen<MessageT>([this](const MessageT& msg) { smlSm_.process_event(msg); });
        }
    }

    /// The adst::Actor class uses the name from the context provided by the user.
    static constexpr auto NAME = ContextT::NAME;

private:
    /// The context class must be created by the user (the class constructor moves it in).
    /// Tt will only be used by the state machine itself.
    ContextT cntx_;
    /// Internal logger class used for logging SML state machine behaviour.
    SmlActorLogger logger_;
    /// The boost:sml::sm is the backend (containing the transition table with context and Actor).
    SmlImpSm smlSm_;
};

template <class SmlSmT, class ContextT>
template <typename T, typename... Rest>
struct SmlActor<SmlSmT, ContextT>::Caller<T, Rest...>
{
    template <typename CallOnT>
    static void call(CallOnT& callOn)
    {
        callOn.template forwardToSml<T>(); // Call the desired forward function with a visited type.
        Caller<Rest...>::call(callOn);
    }
};

template <class SmlSmT, class ContextT>
template <typename T>
struct SmlActor<SmlSmT, ContextT>::Caller<T>
{
    template <typename CallOnT>
    static void call(CallOnT& callOn)
    {
        callOn.template forwardToSml<T>(); // Call the desired forward function with a visited type.
    }
};

} // namespace adst::ep::test_engine::infra
