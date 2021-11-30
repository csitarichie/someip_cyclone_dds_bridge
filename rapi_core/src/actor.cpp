#include <iostream>

#include "core/actor.hpp"

using Actor          = adst::ep::test_engine::core::Actor;
using CallBackHandle = adst::ep::test_engine::core::CallBackHandle;

thread_local int Actor::thread_id;

Actor::Actor(std::string name, const adst::ep::test_engine::core::Environment& env)
    : name_(std::move(name))
    , env_(env)
    , ctorDtorLock_(callBackMutex_)
{
    ADSTLOG_INIT_ACTOR_TRACE_MODULES(name_.c_str());
    scheduled_ = true;
    ctorDtorLock_.unlock(); // allow from here to be receive events.
    ports_.emplace_back(std::make_unique<Port>(*this, env_.network_));
}

void Actor::ctorFinished()
{
    LOG_C_T("lock[ctorDtorLock_] - try to lock");
    ctorDtorLock_.lock(); // now ctor finished check when every scheduling is needed
    LOG_C_T("lock[ctorDtorLock_] - locked");
    if (!callBackQueue_.empty()) {
        LOG_C_D("callBackQueue_ not empty - schedule");
        env_.priority_.schedule([this] { consume(); });
        ctorDtorLock_.unlock(); // make sure that now we can dispatch
        LOG_C_T("lock[ctorDtorLock_] - unlocked");
        LOG_C_D("ctor end");
        return;
    }
    LOG_C_D("callBackQueue_ empty - no schedule");
    scheduled_ = false;
    ctorDtorLock_.unlock(); // really done with construction we unlock. So normal operation
    LOG_C_T("lock[ctorDtorLock_] - unlocked");
    LOG_C_D("ctor end");
}

void Actor::waitForReadyToDtor()
{
    ctorDtorLock_.lock(); // we entering destruction phase no more dispatch allowed.
    if (scheduled_) {
        constDestEvent_.wait(ctorDtorLock_, [this] { return !scheduled_; });
    }
}

void Actor::consume()
{
    std::unique_lock<std::mutex> callBackLock(callBackMutex_);
    LOG_C_D("Actor::consume, callBackQueue.size=%d", callBackQueue_.size());
    while (true) {
        if (!callBackQueue_.empty()) {
            auto callback = callBackQueue_.front();
            callBackQueue_.pop_front();
            callBackLock.unlock();
            callback();
            callBackLock.lock();
        } else {
            scheduled_ = false;
            constDestEvent_.notify_one();
            return;
        }
    }
}

void Actor::schedule(std::function<void()> callBack)
{
    std::lock_guard<std::mutex> callBackGuard(callBackMutex_);
    callBackQueue_.push_back(std::move(callBack));

    // since there is only a single port exist it always changes from 0 to 1 never more.
    // GCOVR_EXCL_START
    if (!scheduled_) {
        // GCOVR_EXCL_STOP
        scheduled_ = true;
        env_.priority_.schedule([this] { consume(); });
    }
}

CallBackHandle Actor::newCallBackHandle()
{
    return ports_[0]->newHandle();
}
