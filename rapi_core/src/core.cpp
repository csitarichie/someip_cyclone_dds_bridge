
#include "core/core.hpp"
#include "core/actor.hpp"

using Core = adst::ep::test_engine::core::Core;

void Core::run()
{
    running_ = true;
    env_.priority_.start();
    env_.priority_.waitForIdle(); // start is delayed as long as all listen is executed from the actors ctor.
    sendStartReq_();
    std::unique_lock<std::mutex> stopLock(stopMutex_);
    stopEvent_.wait(stopLock, [this] { return !running_; });
    env_.priority_.stop();
}
