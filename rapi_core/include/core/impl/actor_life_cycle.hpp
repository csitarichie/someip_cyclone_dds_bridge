#pragma once

#include "core/actor.hpp"
#include "core/environment.hpp"
#include "core/message.hpp"

/**
 * Handle of the startup and shutdown of the actors. The main idea behind this that the user don't
 * have to care about the handle of the startup or the shutdown. The life cycle registers each
 * children to its parent start and stop request, so if the root actor get its start request all of
 * the children actors will be started/stopped too.
 *
 * After finishing a confirm message is broadcasted, if all the children are finsihed aswell.
 */
namespace adst::ep::test_engine::core {
/**
 * Actor mix-in base class for common initialization and destruction. Every real instance has to be
 * constructed of this type.
 *
 * @tparam TActor the type of actor which gets instantiated.
 */
template <typename TActor>
class ActorLifeCycle final : public TActor
{
    using PrivStopReq  = StopReq<TActor>;
    using PrivStopCnf  = Actor::PrivStopCnf<TActor>;
    using PrivStartReq = StartReq<TActor>;
    using PrivStartCnf = Actor::PrivStartCnf<TActor>;
    using PubStartCnf  = StartCnf<TActor>;
    using PubStopCnf   = StopCnf<TActor>;

public:
    /**
     * The constructor registers the actor to the network via listen-publish methods.
     *
     * @param env Environment provided by the core.
     */
    template <typename... Args>
    ActorLifeCycle(const Environment& env, Args&&... args)
        : TActor(env, std::forward<Args>(args)...)
    {
        TActor::lifeCycleHelper_.publishPrivStartCnf_ = [this]() { TActor::publish(std::make_unique<PrivStartCnf>()); };
        TActor::lifeCycleHelper_.publishPrivStopCnf_  = [this]() { TActor::publish(std::make_unique<PrivStopCnf>()); };
        TActor::lifeCycleHelper_.publishPubStartCnf_  = [this]() { TActor::publish(std::make_unique<PubStartCnf>()); };
        TActor::lifeCycleHelper_.publishPubStopCnf_   = [this]() { TActor::publish(std::make_unique<PubStopCnf>()); };

        // storing the type ids in the base for the public start / strop cnf listens
        TActor::lifeCycleHelper_.startCnfTypeId_ = impl::type_id<PubStartCnf>();
        TActor::lifeCycleHelper_.stopCnfTypeId_  = impl::type_id<PubStopCnf>();

        // make sure that from this point onward all listen calls are handled normally
        TActor::SetState(Actor::ActorState::CTOR_FINISHED);

        for (auto& listenIt : TActor::lifeCycleHelper_.delayedListens_) {
            LOG_C_D("delayed listen");
            listenIt();
        }
        TActor::lifeCycleHelper_.delayedListens_.clear();

        // process start req from parent
        TActor::template listen<PrivStartReq>([this](const PrivStartReq&) {
            LOG_C_D("listen:{%s}, TActor::children_.size=%d", PrivStartReq::NAME.c_str(), TActor::children_.size());
            // GCOVR_EXCL_START
            for (auto& it : TActor::children_) {
                // GCOVR_EXCL_STOP
                LOG_C_D("forward StartReq to child");
                it.second.publishStartReq_();
            }

            // in case there is no child StartReq forwarding is finished.
            // GCOVR_EXCL_START
            if (TActor::children_.empty()) {
                // GCOVR_EXCL_STOP
                TActor::SetState(Actor::ActorState::STARTED);
                LOG_C_D("no children, handle own StartCnf, publicStartCallbackList_.size=%d",
                        TActor::lifeCycleHelper_.publicStartCallbackList_.size());
                // in case user was interested in StartCnf
                // GCOVR_EXCL_START
                for (auto const& it : TActor::lifeCycleHelper_.publicStartCallbackList_) {
                    // GCOVR_EXCL_STOP
                    LOG_C_D("handling publicStartCallback");
                    it.second();
                }
                // might be that actors other then self would listen on
                // this actor start cnf -> to know then the actor is ready
                //
                // furthermore, private start cnf to parent
                TActor::lifeCycleHelper_.publishStartCnf();
            }
        });

        TActor::template listen<PrivStopReq>([this](const PrivStopReq&) {
            LOG_C_D("listen:{%s}, TActor::children_.size=%d", PrivStartReq::NAME.c_str(), TActor::children_.size());
            // GCOVR_EXCL_START
            for (auto& it : TActor::children_) {
                // GCOVR_EXCL_STOP
                it.second.publishStopReq_();
            }

            // GCOVR_EXCL_START
            if (TActor::children_.empty()) {
                // GCOVR_EXCL_STOP
                LOG_C_D("no children, handle own StartCnf, publicStopCallbackList_.size=%d",
                        TActor::lifeCycleHelper_.publicStopCallbackList_.size());
                // public stop cnf fnctor
                // GCOVR_EXCL_START
                for (auto const& it : TActor::lifeCycleHelper_.publicStopCallbackList_) {
                    // GCOVR_EXCL_STOP
                    LOG_C_D("handling publicStopCallback");
                    it.second();
                }
                TActor::SetState(Actor::ActorState::STOPPED);
                // this must be sent to inform possible listener in other actor about stopCnf
                //
                // furthermore, private stop cnf to parent
                TActor::lifeCycleHelper_.publishStopCnf();
            }
        });

        TActor::ctorFinished();
    }

    /**
     * Destructor with the mandatory shutdown synchronisation callback to Actor base class.
     */
    ~ActorLifeCycle()
    {
        TActor::waitForReadyToDtor();
    }
};
} // namespace adst::ep::test_engine::core
