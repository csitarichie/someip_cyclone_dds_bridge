#pragma once
#include "core/environment.hpp"
#include "core/impl/actor_life_cycle.hpp"
#include "core/priority.hpp"

namespace adst::ep::test_engine::core {

/**
 * main entry point to stat the ADST test engine
 * this is the class which is instantiated from main
 * run() method does the processes till the system not stopped.
 */
class Core final
{
public:
    // not copyable or movable
    Core(const Core&) = delete;
    Core(Core&&)      = delete;
    Core()            = delete;

    Core& operator=(const Core&) = delete;
    Core& operator=(Core&&) = delete;

    /**
     * Core can be created in main. It needs the config class which holds the runtime configuration
     * @param config
     */
    explicit Core(const ConfigAndLogger& configAndLogger)
        : env_(configAndLogger)
    {
        ADSTLOG_INIT_ACTOR_TRACE_MODULES("Core");
        ADSTLOG_REGISTER_THREAD(0, "main");
    }

    /**
     * After core creation, the init(..) function must be called with the root actor.
     *
     * @tparam TRootActor The actor which acts as a root actor.
     * @tparam Args Possible constructor parameter types perfect forwarded to the new RootActor
     * @param args Actual constructor argument list. Note: The first argument env is mandatory for
     *        all Actor.
     */
    template <typename TRootActor, typename... Args>
    void init(Args&&... args)
    {
        static_assert(std::is_base_of<Actor, TRootActor>::value, "TRootActor must be derived from Actor");

        using PrivStopReq  = StopReq<TRootActor>;
        using PrivStopCnf  = Actor::PrivStopCnf<TRootActor>;
        using PrivStartReq = StartReq<TRootActor>;

        root_         = std::make_unique<ActorLifeCycle<TRootActor>>(env_, std::forward<Args>(args)...);
        sendStartReq_ = [this]() { env_.network_.publish(std::make_unique<PrivStartReq>()); };

        root_->listen<Stop>([this](const Stop&) {
            LOG_C_D("core received Stop");
            env_.network_.publish(std::make_unique<PrivStopReq>());
        });

        root_->listen<PrivStopCnf>([this](const PrivStopCnf&) {
            LOG_C_D("core received %s", PrivStopCnf::NAME.c_str());
            {
                std::lock_guard<std::mutex> stopGuard(stopMutex_);
                running_ = false;
            }
            stopEvent_.notify_one();
        });
    }

    /**
     * Start executing the entire system. Note: run() blocks the calling tread until the stop message gets published.
     */
    void run();

    const Environment& getEnv() const
    {
        return env_;
    }

private:
    const Environment env_;       /// Shared environment for all actors.
    Actor::ActorPtr   root_ = {}; /// The root actor created by init(..).
    // this is a cumbersome ugly pattern what std::threads forcing on you to create and use a
    // pattern to wait for an event and sleep till it is signalled.
    // you need a mutex for wait ...
    // you need a conditional for signalling
    // you need a guard variable which represents a unique change to able to distinguish between real signalled event.
    // and false positive wakeup for OS.
    std::mutex              stopMutex_ = {};    /// used by wait for stop event.
    std::condition_variable stopEvent_ = {};    /// fired by stop callback
    bool                    running_   = false; /// used for guarding the stop wait wakeup.
    std::function<void()>   sendStartReq_;
    ADSTLOG_DEF_ACTOR_TRACE_MODULES();
};

} // namespace adst::ep::test_engine::core
