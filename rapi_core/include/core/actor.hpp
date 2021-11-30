#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "adstutil_cxx/error_handler.hpp"

#include "core/core_types.hpp"
#include "core/environment.hpp"
#include "core/message.hpp"

namespace adst::ep::test_engine::core {

class Priority;

class Port;

template <typename TActor>
class ActorLifeCycle;

/**
 * Actor is a main base class of all entities which are communicating in the ADST test engine core.
 * It provides the infrastructure for publishing and listening to messages.
 */
class Actor
{
public:
    // not copyable or movable
    Actor(const Actor&) = delete;
    Actor& operator=(const Actor&) = delete;
    Actor(Actor&&)                 = delete;
    Actor& operator=(Actor&&) = delete;

    virtual ~Actor() = default;

    using ChildHandle = size_t;                /// Children are identified by a unique handle.
    using PortPtr     = std::unique_ptr<Port>; /// Ports are owned by actors.
    using PortList    = std::vector<PortPtr>;  /// Store for the ports.
    using PortIndex   = size_t;                /// Ports are indexed by PortIndex.

    /**
     * Actor are always constructed by providing a name and the ADST environment.
     * @param name Name of the derived actor.
     * @param env Environment provided by the core.
     */
    explicit Actor(std::string name, const Environment& env);

    /**
     * Shall be called at the beginning of derived class destructors to make sure that the actor has
     * finished processing messages and is ready to be destroyed.
     */
    void waitForReadyToDtor();

    /**
     * Each actor has a name (which e.g. can be used for tracing).
     * @return Name of the actor
     */
    inline const std::string& getName() const
    {
        return name_;
    }

    /**
     * Provide access to the Environment of this actor.
     *
     * @return Global read only environment for components to get config and handle trace and errors.
     */
    // GCOVR_EXCL_START
    inline const Environment& getEnv() const
    {
        return env_;
    }
    // GCOVR_EXCL_STOP

    /**
     * Derived class constructors shall call this function to inform Actor base class that
     * construction has finished.
     */
    void ctorFinished();

    /**
     * TMessage is broadcasted to all actors subscribed to listen to a message identified by
     * TMessage.
     *
     * @tparam TMessage The type of the message to be broadcasted. The function expects a unique_ptr
     *         with this type.
     * @param msg A unique_ptr to a message instance. The ownership of msg is transferred from the
     *        publisher to the actor.
     */
    template <typename TMessage>
    void publish(std::unique_ptr<TMessage> msg)
    {
        LOG_CH_D(MSG_TX, "%s", impl::getTypeName<TMessage>().c_str());
        if constexpr (!std::is_base_of<PrivStartHelper, TMessage>::value &&
                      !std::is_base_of<PrivStopHelper, TMessage>::value &&
                      !std::is_base_of<ReqHelper, TMessage>::value &&
                      !std::is_base_of<StopCnfHelper, TMessage>::value) {
            if (state_ != ActorState::STARTED) {
                env_.configAndLogger_.config_.onErrorCallBack_(adst::common::Error{
                    {5, fmt::format("{}.publish<{}>(...) called in '{}' state, publish only allowed in STARTED",
                                    getName(), impl::getTypeName<TMessage>(), actorStateToStr(state_))}});
            }
        }

        env_.network_.publish(std::move(msg));
    }

    /**
     * Registers a callback function for listening to messages with TMessage type.
     * @tparam TMessage Message type to listen to.
     * @param callBack Callback function will be called with an instance of TMessage when a publish happened,
     * @return A unique handle which can be used to unlisten.
     */
    template <typename TMessage>
    CallBackHandle listen(std::function<void(const TMessage&)> callBack);

    /**
     * Adds the listen with a predefined handle. Use if you want to unlisten in the callback
     * @tparam TMessage Message type to listen to.
     * @param handle The unique handle which can be used to unlisten.
     * @param callBack Callback function will be called with an instance of TMessage when a publish happened,
     */
    template <typename TMessage>
    void listen(CallBackHandle handle, std::function<void(const TMessage&)> callBack);

    /**
     * Unregisters the callback function which is identified by handle.
     * @tparam TMessage The message type to look for.
     * @param handle The handle identifying the callback function.
     */
    template <typename TMessage>
    void unlisten(const CallBackHandle handle);

    /**
     * Creates an actor of type TChild.
     *
     * It is safe to create children in constructor of an actor and start sending messages to the
     * child in start Callback. Creating the child and sending a message to a child at the same
     * transition is up for a race. since the registration of any listen fo child port happens when
     * the child is scheduled so a later point in time.
     *
     * To avoid it a pattern needs to be used:
     * - parent registers to a child created message
     * - parent creates child
     * - child sends created message to parent
     * - parent sends start to child in child created transaction.
     *
     * @tparam TChild The user provided type for the actor to be created.
     * @tparam Args Possible constructor parameter types perfect forwarded to the new child
     * @param args Actual constructor argument list. Note: The first argument env is mandatory for
     *        all children.
     * @return Unique handle identifying the newly created TChild.
     */
    template <typename TChild, typename... Args>
    ChildHandle newChild(Args&&... args);

    /**
     * Gets a reference to a child identified by the handle.
     * @param handle Unique identifier assigned during child creation.
     * @return Const reference to the child actor.
     */
    const Actor& getChild(ChildHandle handle) const
    {
        auto child = children_.find(handle);
        if (child == children_.end()) {
            env_.configAndLogger_.config_.onErrorCallBack_(adst::common::Error{{5, "child handle not found"}});
        }
        return *child->second.child_;
    }

#ifdef ADST_CORE_TEST_ENABLED
    /**
     * Only required (and shall only be used) by tests.
     * @param index Index of the port (as of today only 0 is valid).
     * @return Reference to the port selected by index.
     */
    Port& getPortForTest(PortIndex index) const
    {
        return *ports_[index];
    }
#endif

    ADSTLOG_DEF_ACTOR_TRACE_MODULES();

protected:
    /**
     *
     */
    int newCallBackHandle();

    /**
     * @return The number of the actor's children.
     */
    size_t getChildCount() const
    {
        return children_.size();
    }

    const std::string name_ = "Actor"; /// stores the name of the actor.

    /**
     * Core provided environment contains runtime configuration and system wide parameters.
     */
    const Environment& env_;

private:
    using ActorPtr = std::unique_ptr<Actor>; /// Actors can own Actors. Ownership is always unique.

    /**
     * Describe Actor children. All the children have a pointer to access them.
     * We also store the publish start and stop request callbacks while we know the type of the child.
     */
    struct ChildContainer
    {
        std::function<void()> publishStartReq_; /// the start request callback
        std::function<void()> publishStopReq_;  /// the stop request callback
        ActorPtr              child_;           /// pointer to the child
    };

    using ActorList = std::unordered_map<ChildHandle, ChildContainer>; /// Store for the children.

    struct LifeCycleHelper
    {
        /**
         * Informs public and parent listeners about start finish for its own start and all children
         * start.
         */
        inline void publishStartCnf()
        {
            publishPubStartCnf_();
            publishPrivStartCnf_();
        }

        /**
         * Informs public and parent listeners about stop finish for its own stop and all children
         * stop.
         */
        inline void publishStopCnf()
        {
            publishPubStopCnf_();
            publishPrivStopCnf_();
        }

        using CallBackList   = std::map<CallBackHandle, std::function<void()>>;
        using CallBackVector = std::vector<std::function<void()>>;

        impl::type_id_t startCnfTypeId_ = impl::type_id<void>(); /// type of derived start cnf
        impl::type_id_t stopCnfTypeId_  = impl::type_id<void>(); /// type of derived stop cnf
        CallBackList    publicStartCallbackList_;                /// stores functions to call right after startCnf
        CallBackList    publicStopCallbackList_;                 /// stores functions to call right after stopCnf
        CallBackVector  delayedListens_;                         /// stores listen callbacks till ctor finished
        /// call this when all the children have started for public listeners actors other than this.
        std::function<void()> publishPubStartCnf_;
        /// call this when all the children have stopped for public listeners actors other than this
        std::function<void()> publishPubStopCnf_;
        /// call this when all the children have started to parent
        std::function<void()> publishPrivStartCnf_;
        /// call this when all the children have stoped to parent
        std::function<void()> publishPrivStopCnf_;
        unsigned int          childrenCnfCount_ = 0; /// the number of children that has finished starting / stopping
    };

    struct PrivStartHelper
    {
    };

    template <typename ActorT>
    struct PrivStartCnf : public PrivStartHelper
    {
        static constexpr auto NAME = "PrivStartCnf<" + ActorT::NAME + ">";
    }; // struct PrivStartCnf

    struct PrivStopHelper
    {
    };

    template <typename ActorT>
    struct PrivStopCnf : public PrivStopHelper
    {
        static constexpr auto NAME = "PrivStopCnf<" + ActorT::NAME + ">";
    }; // struct PrivStopCnf

    friend Priority;
    friend Port;
    friend Core;

    template <typename Actor>
    friend class ActorLifeCycle;

    /**
     * Actors execute sequence of actions based on current state and internal events see bellow.
     */
    enum class ActorState
    {
        INIT,          /// initial state when the actor is only constructed
        CTOR_FINISHED, /// state entered when finished construction
        STARTED,       /// state entered after start confirm (all the children started)
        STOPPED,       /// state entered after stop confirm (all the children started)
    };

    inline void SetState(ActorState state)
    {
        if (state != state_) {
            LOG_C_D("'%s' -> '%s'", actorStateToStr(state_), actorStateToStr(state));
            state_ = state;
        }
    }

    inline const char* actorStateToStr(ActorState state)
    {
        switch (state) {
            case ActorState::INIT:
                static const auto init_name = sstr::literal("INIT");
                return init_name;
            case ActorState::CTOR_FINISHED:
                static const auto ctor_finished_name = sstr::literal("CTOR_FINISHED");
                return ctor_finished_name;
            case ActorState::STARTED:
                static const auto started_name = sstr::literal("STARTED");
                return started_name;
            case ActorState::STOPPED:
                static const auto stopped_name = sstr::literal("STOPPED");
                return stopped_name;
                // GCOVR_EXCL_START
            default:
                static const auto invalid_name = sstr::literal("INVALID");
                return invalid_name;
                // GCOVR_EXCL_STOP
        }
    }

    /**
     * Consumes all callbacks in the queue. This functions is called from priority.
     */
    void consume();

    /**
     * Schedules actor on the priority.
     */
    void schedule(std::function<void()>);

    /**
     * Creates a new unique value for a child.
     * @return
     */
    ChildHandle newHandle()
    {
        return ++childHandleCounter_;
    }

    ChildHandle childHandleCounter_ = 0; /// counter implements unique id handling in child creation

    // Only the queue is protected against multi threading since the only function allowed to be called
    // from other thread is schedule(..). All other functions shall only be called via callbacks.
    //
    // Callbacks are consumed as long as callBackQueue_ is not empty.
    //
    // Synchronisation is needed for destructor and for constructor calls since it could be that
    // while constructor is running a schedule is called because a message has been sent to the
    // actor. Therefore scheduling of this case shall be postponed.
    //
    // Same story for destructor when there is a callback running and we call the destructor it
    // shall wait until the callback / queue is consumed.
    std::condition_variable           constDestEvent_;  /// signals unlock scheduler and dest
    bool                              scheduled_;       /// true when Actor is scheduled or executing a callback
    std::deque<std::function<void()>> callBackQueue_;   /// stores ports which have work to do
    std::mutex                        callBackMutex_;   /// protects the callBackQueue_
    std::unique_lock<std::mutex>      ctorDtorLock_;    /// used by ctor and dtor sync with derived callers.
    PortList                          ports_ = {};      /// Actor input ports as of now only get created
    LifeCycleHelper                   lifeCycleHelper_; /// sticks together the data of the life cycle

    /**
     * Thread local id gets filled by Priority with the currently executing thread id. The main
     * purpose of this id is logging.
     */
    thread_local static int thread_id;

    ActorList  children_ = {};               /// Sorting children by handle
    ActorState state_    = ActorState::INIT; /// state initialized to INIT
};
} // namespace adst::ep::test_engine::core

// it has to happen here due to template and circular dependency
#include "core/impl/actor_life_cycle.hpp"
#include "core/port.hpp"

namespace adst::ep::test_engine::core {

template <typename TChild, typename... Args>
Actor::ChildHandle Actor::newChild(Args&&... args)
{
    using PrivStartReq  = StartReq<TChild>;
    using PrivStartCnfT = PrivStartCnf<TChild>;
    using PrivStopReq   = StopReq<TChild>;
    using PrivStopCnfT  = PrivStopCnf<TChild>;
    static_assert(std::is_base_of<Actor, TChild>::value, "TRootActor must be derived from Actor");

    auto handle = newHandle();
    children_.emplace(handle,
                      ChildContainer{[this]() { publish(std::make_unique<PrivStartReq>()); },
                                     [this]() { publish(std::make_unique<PrivStopReq>()); },
                                     std::make_unique<ActorLifeCycle<TChild>>(env_, std::forward<Args>(args)...)});
    // GCOVR_EXCL_START
    listen<PrivStartCnfT>([this](const PrivStartCnfT&) {
        lifeCycleHelper_.childrenCnfCount_++;
        if (lifeCycleHelper_.childrenCnfCount_ == children_.size()) {
            lifeCycleHelper_.childrenCnfCount_ = 0;
            SetState(ActorState::STARTED);
            // public start cnf fnctor
            for (auto const& it : lifeCycleHelper_.publicStartCallbackList_) {
                it.second();
            }
            // GCOVR_EXCL_STOP

            // public and private start cnf
            lifeCycleHelper_.publishStartCnf();
        }
    });
    // GCOVR_EXCL_START
    listen<PrivStopCnfT>([this](const PrivStopCnfT&) {
        lifeCycleHelper_.childrenCnfCount_++;
        if (lifeCycleHelper_.childrenCnfCount_ == children_.size()) {
            lifeCycleHelper_.childrenCnfCount_ = 0;
            SetState(ActorState::STOPPED);
            // public stop cnf fnctor
            for (auto const& it : lifeCycleHelper_.publicStopCallbackList_) {
                it.second();
            }
            // GCOVR_EXCL_STOP

            // public and private stop cnf
            lifeCycleHelper_.publishStopCnf();
        }
    });
    return handle;
}

template <typename TMessage>
void Actor::listen(CallBackHandle handle, std::function<void(const TMessage&)> callBack)
{
    if constexpr (std::is_base_of<core::StartCnfHelper, TMessage>::value) {
        if (state_ == ActorState::INIT) { // type id is not yet initialized
            LOG_C_D("delayed listen:{%s}", impl::getTypeName<TMessage>().c_str());
            lifeCycleHelper_.delayedListens_.emplace_back([this, handle, callBack]() {
                // GCOVR_EXCL_START
                if (impl::type_id<TMessage>() == lifeCycleHelper_.startCnfTypeId_) {
                    // GCOVR_EXCL_STOP
                    lifeCycleHelper_.publicStartCallbackList_.insert(
                        {handle, std::move([this, callBack]() {
                             LOG_CH_D(MSG_RX, "%s", impl::getTypeName<TMessage>().c_str());
                             callBack(TMessage());
                         })});
                    LOG_C_D("added delayed listener:{%s} to publicStartCallbackList_, size=%d",
                            impl::getTypeName<TMessage>().c_str(), lifeCycleHelper_.publicStartCallbackList_.size());
                } else {
                    ports_[0]->listen(handle, std::move(callBack));
                }
            });
        } else { // when listen for StartCnf called after constructor
            LOG_C_D("special listen:{%s}", impl::getTypeName<TMessage>().c_str());
            if (impl::type_id<TMessage>() == lifeCycleHelper_.startCnfTypeId_) {
                LOG_C_D("listen:{%s}, matches local StartCnf", impl::getTypeName<TMessage>().c_str());
                lifeCycleHelper_.publicStartCallbackList_.insert({handle, std::move([this, callBack]() {
                                                                      LOG_CH_D(MSG_RX, "%s",
                                                                               impl::getTypeName<TMessage>().c_str());
                                                                      callBack(TMessage());
                                                                  })});
                LOG_C_D("added listener:{%s} to publicStartCallbackList_, size=%d",
                        impl::getTypeName<TMessage>().c_str(), lifeCycleHelper_.publicStartCallbackList_.size());
            } else {
                ports_[0]->listen(handle, std::move(callBack));
            }
        }
    }

    else if constexpr (std::is_base_of<core::StopCnfHelper, TMessage>::value) {
        if (state_ == ActorState::INIT) { // type id is not yet initialized
            LOG_C_D("delayed listen:{%s}", impl::getTypeName<TMessage>().c_str());
            lifeCycleHelper_.delayedListens_.emplace_back([this, handle, callBack]() {
                // GCOVR_EXCL_START
                if (impl::type_id<TMessage>() == lifeCycleHelper_.stopCnfTypeId_) {
                    // GCOVR_EXCL_STOP
                    lifeCycleHelper_.publicStopCallbackList_.insert(
                        {handle, std::move([this, callBack]() {
                             LOG_CH_D(MSG_RX, "%s", impl::getTypeName<TMessage>().c_str());
                             callBack(TMessage());
                         })});
                    LOG_C_D("added delayed listener:{%s} to publicStopCallbackList_, size=%d",
                            impl::getTypeName<TMessage>().c_str(), lifeCycleHelper_.publicStopCallbackList_.size());
                } else {
                    ports_[0]->listen(handle, std::move(callBack));
                }
            });
        } else {
            LOG_C_D("special listen:{%s}", impl::getTypeName<TMessage>().c_str());
            if (impl::type_id<TMessage>() == lifeCycleHelper_.stopCnfTypeId_) {
                LOG_C_D("listen:{%s}, matches local StopCnf", impl::getTypeName<TMessage>().c_str());
                lifeCycleHelper_.publicStopCallbackList_.insert({handle, std::move([this, callBack]() {
                                                                     LOG_CH_D(MSG_RX, "%s",
                                                                              impl::getTypeName<TMessage>().c_str());
                                                                     callBack(TMessage());
                                                                 })});
                LOG_C_D("added listener:{%s} to publicStopCallbackList_, size=%d",
                        impl::getTypeName<TMessage>().c_str(), lifeCycleHelper_.publicStopCallbackList_.size());
            } else {
                ports_[0]->listen(handle, std::move(callBack));
            }
        }
    }

    else {
        // for now we only have 1 port.
        ports_[0]->listen(handle, std::move(callBack));
    }
}

template <typename TMessage>
CallBackHandle Actor::listen(std::function<void(const TMessage&)> callBack)
{
    if constexpr (std::is_base_of<core::StartCnfHelper, TMessage>::value ||
                  std::is_base_of<core::StopCnfHelper, TMessage>::value) {
        auto newHandle = newCallBackHandle();
        listen<TMessage>(newHandle, std::move(callBack));
        return newHandle;
    }

    // for now we only have 1 port.
    return ports_[0]->listen(std::move(callBack));
}

template <typename TMessage>
void Actor::unlisten(CallBackHandle handle)
{
    // for now we only have 1 port.
    if constexpr (std::is_base_of<core::StartCnfHelper, TMessage>::value) {
        LOG_C_D("special unlisten:{%s}", impl::getTypeName<TMessage>().c_str());
        if (impl::type_id<TMessage>() == lifeCycleHelper_.startCnfTypeId_) {
            lifeCycleHelper_.publicStartCallbackList_.erase(handle);
            return;
        } // else we return to the default unlisten
    }

    if constexpr (std::is_base_of<core::StopCnfHelper, TMessage>::value) {
        LOG_C_D("special unlisten:{%s}", impl::getTypeName<TMessage>().c_str());
        if (impl::type_id<TMessage>() == lifeCycleHelper_.stopCnfTypeId_) {
            lifeCycleHelper_.publicStopCallbackList_.erase(handle);
            return;
        } // else we return to the default unlisten
    }
    ports_[0]->unlisten<TMessage>(handle);
}

} // namespace adst::ep::test_engine::core
