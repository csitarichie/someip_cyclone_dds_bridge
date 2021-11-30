#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include "adstlog_cxx/adstlog.hpp"

#include "adstutil_cxx/error_handler.hpp"

namespace adst::ep::test_engine::core {

/**
 * * Type to be used for callbacks on priority. Shall be a simple void function.
 */
using CallBack = std::function<void()>;

class Priority final
{
public:
    // not copyable or movable
    Priority(const Priority&) = delete;
    Priority(Priority&&)      = delete;

    Priority& operator=(const Priority&) = delete;
    Priority& operator=(Priority&&) = delete;

    /**
     * Creates dispatchers and handles startup and shutdown of the system in a graceful way.
     * @param dispatcherCount Number of parallel dispatchers to run.
     * @param onErrorCallBack Function to be be called in case of errors.
     */
    Priority(int dispatcherCount, const adst::common::OnErrorCallBack& onErrorCallBack);

    ~Priority();

    /**
     * Starts the dispatchers. Blocks until all dispatcher are up and running.
     */
    void start();

    /**
     * Stops all dispatchers. Blocks until all callbacks are dispatched.
     * It is called after stop event is published.
     */
    void stop();

    /**
     * Schedules a callback on one of the free dispatchers.
     * @param callBack function to schedule on one of the dispatcher
     */
    void schedule(CallBack callBack);

    /**
     * Blocks calling thread until all dispatcher thread in idle state, eg no callback to dispatch.
     */
    void waitForIdle();

private:
    /**
     * Internal start event it takes the lock as input since in destructor this is locked for more than one start stop
     * sequence
     * @param lock locked created by the caller either public stop or by destructor
     */
    void start(std::unique_lock<std::mutex>& lock);

    /**
     * Internal stop event it takes the lock as input since in destructor this is locked for more than one start stop
     * sequence
     * @param lock locked created by the caller either public stop or by destructor
     */
    void stop(std::unique_lock<std::mutex>& lock);

    /**
     * function runs on the thread context preforms dispatching of callbacks.
     * @param threadId
     */
    inline void dispatcherLoop(int threadId);

    /**
     * Priority executes sequence of actions based on current state and internal events see  bellow.
     */
    enum class State
    {
        START,    /// initial state when the priority is only constructed
        RUNNING,  /// state entered during `run` function call
        STOPPING, /// state entered at the beginning of `stop` function
        STOPPED,  /// when all tread are exited and priority is ready to be destroyed.
    };
    // Start stop synchronization is done via conditional and atomic.
    // An alternative would have been to use command pattern,
    // but for that 2 queue or polymorphic queue element would have been necessary.
    // Since the only command for now is STOP, both solution would have been overkill.

    const adst::common::OnErrorCallBack& onErrorCallBack_;  /// error callback from environment no return
    std::vector<std::thread>             dispatchers_ = {}; /// store dispatcher threads
    std::deque<CallBack>                 actorQueue_  = {}; /// the CallBack queue to be dispatched in dispatcher thread
    std::mutex stateChangeMutex_                      = {}; /// guard for internal state change queue or state variable
    /// either when the Actor queue changes form empty or when Start or Stop signalled
    /// this signal wakes up the dispatcher threads
    std::condition_variable scheduleEvent_ = {};
    /// Used the dispatchers to signal that they are waiting for dispatching used during start
    /// as well when for stop function and destructor synchronization.
    std::condition_variable startEvent_   = {}; /// signal start to dispatchers
    std::mutex              startedMutex_ = {}; /// block start function
    std::condition_variable startedEvent_ = {}; /// signal that dispatcher started
    int                     startCount_   = 0;  /// guard variable to unlock start
    /// a change from STARTING to RUNNING triggers startedEvent_ -> Main to dispatchers,
    /// a change from RUNNING to STOPPED triggers again startedEvent_ -> last dispatcher to Main,
    State state_ = State::START;
    /// using it for idle wait.
    /// incremented/decremented when a dispatcher thread goes to/from sleep
    size_t                  sleepingCount_ = 0;
    std::condition_variable idleEvent_     = {}; /// signal when all dispatcher sleeps.
    ADSTLOG_DEF_ACTOR_TRACE_MODULES();
};

} // namespace adst::ep::test_engine::core
