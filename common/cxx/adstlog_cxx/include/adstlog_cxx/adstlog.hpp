#pragma once

#include <memory>
#include <string>
#include <vector>

#include "adstlog_cxx/p7_trace_wrapper.hpp"

// clang-format off
// Actor module registers all of these channels
#define ADSTLOG_ACTOR_CH   p7ActorCh_
#define ADSTLOG_MSG_TX_CH  p7MsgTxCh_
#define ADSTLOG_MSG_RX_CH  p7MsgRxCh_
#define ADSTLOG_GRPC_RX_CH p7GrpcRxCh_
#define ADSTLOG_GRPC_TX_CH p7GrpcTxCh_
#define ADSTLOG_GRPC_CH    p7GrpcCh_
#define ADSTLOG_CORE_CH    p7CoreCh_

// channel names to be used for set trace level on channels
#define ADSTLOG_MSG_TX_CH_NAME  "msg_tx"  // message published internal dispatch
#define ADSTLOG_MSG_RX_CH_NAME  "msg_rx"  // message consumed internal dispatch
#define ADSTLOG_GRPC_RX_CH_NAME "grpc_rx" // message received between processes
#define ADSTLOG_GRPC_TX_CH_NAME "grpc_tx" // message transmitted between processes
#define ADSTLOG_GRPC_CH_NAME    "grpc"    // used in gRPC boarder actors debugging
#define ADSTLOG_CORE_CH_NAME    "core"    // used by all framework classes
#define ADSTLOG_ACTOR_CH_NAME   "actor"   // all user modules are on this channel

// helper to be used in for loops to iterate over on channels.
#define ADSTLOG_TRACE_CHANNELS \
    ADSTLOG_MSG_TX_CH_NAME,    \
    ADSTLOG_MSG_RX_CH_NAME,    \
    ADSTLOG_GRPC_RX_CH_NAME,   \
    ADSTLOG_GRPC_TX_CH_NAME,   \
    ADSTLOG_GRPC_CH_NAME,      \
    ADSTLOG_CORE_CH_NAME,      \
    ADSTLOG_ACTOR_CH_NAME

// the actual call for the trace function
#define ADSTLOG_LOG_ON(OBJ, LVL, CHANNEL, MODULE, ...) \
    OBJ->CHANNEL->getObj()->Trace(0,                   \
                   LVL,                                \
                   OBJ->MODULE,                        \
                   (tUINT16)__LINE__,                  \
                   (const char*)__FILE__,              \
                   (const char*)__FUNCTION__,          \
                   __VA_ARGS__)

// then the logging is happening in a class where trace defined
#define ADSTLOG_LOG(...)   \
    ADSTLOG_LOG_ON(this, __VA_ARGS__)

// helper macros
#define ADSTLOG_CH(CH) ADSTLOG_##CH##_CH
#define ADSTLOG_CH_MOD(CH, MOD) p7_##CH##_##MOD##_

// Helper macros for define / declare traces in actor base
#define ADSTLOG_GET_CH(CH) mutable adst::common::Logger::ChannelUPtr ADSTLOG_CH(CH) = \
    std::make_unique<adst::common::ManageTraceObj<IP7_Trace>>(&P7_Get_Shared_Trace,  \
        ADSTLOG_##CH##_CH_NAME, &IP7_Trace::Release)

#define ADSTLOG_DEFINE_CH(CH) mutable adst::common::Logger::ChannelUPtr ADSTLOG_CH(CH) = nullptr

#define ADSTLOG_INIT_CH(CH) ADSTLOG_CH(CH) =                                         \
    std::make_unique<adst::common::ManageTraceObj<IP7_Trace>>(&P7_Get_Shared_Trace, \
        ADSTLOG_##CH##_CH_NAME, &IP7_Trace::Release)

#define ADSTLOG_REGISTER_MODULE(CH, MOD, NAME)                                                                 \
    mutable IP7_Trace::hModule ADSTLOG_CH_MOD(CH, MOD) = nullptr;                                              \
    tBOOL isNewModule_##CH##_##MOD = ADSTLOG_CH(CH)->getObj()->Register_Module(NAME, &ADSTLOG_CH_MOD(CH, MOD))

#define ADSTLOG_DEFINE_MODULE(CH, MOD) mutable IP7_Trace::hModule ADSTLOG_CH_MOD(CH, MOD) = nullptr

#define ADSTLOG_INIT_MODULE(CH, MOD, NAME) ADSTLOG_CH(CH)->getObj()->Register_Module(NAME, &ADSTLOG_CH_MOD(CH, MOD))

#define TEST_HELPER_ADSTLOG_UNUSED(CH, MOD) \
    mutable (void)isNewModule_##CH##_##MOD

// public macros
#define TEST_HELPER_ADSTLOG_UNUSED_ALL()        \
    TEST_HELPER_ADSTLOG_UNUSED(ACTOR, ACTOR);   \
    TEST_HELPER_ADSTLOG_UNUSED(MSG_TX, ACTOR);  \
    TEST_HELPER_ADSTLOG_UNUSED(MSG_RX, ACTOR);  \
    TEST_HELPER_ADSTLOG_UNUSED(GRPC_RX, ACTOR); \
    TEST_HELPER_ADSTLOG_UNUSED(GRPC_TX, ACTOR); \
    TEST_HELPER_ADSTLOG_UNUSED(GRPC, ACTOR);    \
    TEST_HELPER_ADSTLOG_UNUSED(CORE, ACTOR)

/**
 * Generic trace macros can be used in ADST FW to log on non actor channel and non actor.name_ module.
 * The need for this is very rare. Normally, user shall not use these.
 *
 * They are used by the more restrictive trace macros like LOG_I(...), and LOG_CH_I(...), etc.
 */
#define LOG_CH_MOD_E(CH, MOD, ...) \
    ADSTLOG_LOG(EP7TRACE_LEVEL_ERROR,   ADSTLOG_CH(CH), ADSTLOG_CH_MOD(CH, MOD), __VA_ARGS__) /* NOLINT */
#define LOG_CH_MOD_W(CH, MOD, ...) \
    ADSTLOG_LOG(EP7TRACE_LEVEL_WARNING, ADSTLOG_CH(CH), ADSTLOG_CH_MOD(CH, MOD), __VA_ARGS__) /* NOLINT */
#define LOG_CH_MOD_I(CH, MOD, ...) \
    ADSTLOG_LOG(EP7TRACE_LEVEL_INFO,    ADSTLOG_CH(CH), ADSTLOG_CH_MOD(CH, MOD), __VA_ARGS__) /* NOLINT */
#define LOG_CH_MOD_D(CH, MOD, ...) \
    ADSTLOG_LOG(EP7TRACE_LEVEL_DEBUG,   ADSTLOG_CH(CH), ADSTLOG_CH_MOD(CH, MOD), __VA_ARGS__) /* NOLINT */
#define LOG_CH_MOD_T(CH, MOD, ...) \
    ADSTLOG_LOG(EP7TRACE_LEVEL_TRACE,   ADSTLOG_CH(CH), ADSTLOG_CH_MOD(CH, MOD), __VA_ARGS__) /* NOLINT */

#define LOG_CH_MOD_E_ON(OBJ, CH, MOD, ...) \
    ADSTLOG_LOG_ON(OBJ, EP7TRACE_LEVEL_ERROR,   ADSTLOG_CH(CH), ADSTLOG_CH_MOD(CH, MOD), __VA_ARGS__) /* NOLINT */
#define LOG_CH_MOD_W_ON(OBJ, CH, MOD, ...) \
    ADSTLOG_LOG_ON(OBJ, EP7TRACE_LEVEL_WARNING, ADSTLOG_CH(CH), ADSTLOG_CH_MOD(CH, MOD), __VA_ARGS__) /* NOLINT */
#define LOG_CH_MOD_I_ON(OBJ, CH, MOD, ...) \
    ADSTLOG_LOG_ON(OBJ, EP7TRACE_LEVEL_INFO,    ADSTLOG_CH(CH), ADSTLOG_CH_MOD(CH, MOD), __VA_ARGS__) /* NOLINT */
#define LOG_CH_MOD_D_ON(OBJ, CH, MOD, ...) \
    ADSTLOG_LOG_ON(OBJ, EP7TRACE_LEVEL_DEBUG,   ADSTLOG_CH(CH), ADSTLOG_CH_MOD(CH, MOD), __VA_ARGS__) /* NOLINT */
#define LOG_CH_MOD_T_ON(OBJ, CH, MOD, ...) \
    ADSTLOG_LOG(OBJ, EP7TRACE_LEVEL_TRACE,   ADSTLOG_CH(CH), ADSTLOG_CH_MOD(CH, MOD), __VA_ARGS__) /* NOLINT */

/**
 * Log macro meant to be used within the ADST FW for tracing
 * on actor.name_ channel instead of actor or core.
 *
 * Mostly used in fw to log gRPC and dispatch message tx/rx.
 */
#define LOG_CH_E(CH, ...) LOG_CH_MOD_E(CH, ACTOR, __VA_ARGS__)
#define LOG_CH_W(CH, ...) LOG_CH_MOD_W(CH, ACTOR, __VA_ARGS__)
#define LOG_CH_I(CH, ...) LOG_CH_MOD_I(CH, ACTOR, __VA_ARGS__)
#define LOG_CH_D(CH, ...) LOG_CH_MOD_D(CH, ACTOR, __VA_ARGS__)
#define LOG_CH_T(CH, ...) LOG_CH_MOD_T(CH, ACTOR, __VA_ARGS__)

#define LOG_CH_E_ON(OBJ, CH, ...) LOG_CH_MOD_E_ON((&OBJ), CH, ACTOR, __VA_ARGS__)
#define LOG_CH_W_ON(OBJ, CH, ...) LOG_CH_MOD_W_ON((&OBJ), CH, ACTOR, __VA_ARGS__)
#define LOG_CH_I_ON(OBJ, CH, ...) LOG_CH_MOD_I_ON((&OBJ), CH, ACTOR, __VA_ARGS__)
#define LOG_CH_D_ON(OBJ, CH, ...) LOG_CH_MOD_D_ON((&OBJ), CH, ACTOR, __VA_ARGS__)
#define LOG_CH_T_ON(OBJ, CH, ...) LOG_CH_MOD_T_ON((&OBJ), CH, ACTOR, __VA_ARGS__)

/**
 * Logging macro meant to be used within the ADST FW for logging on core channel and on actor.name_ module
 * main macro used for tracing within the fw for debug
 */
#define LOG_C_E(...) LOG_CH_E(CORE, __VA_ARGS__)
#define LOG_C_W(...) LOG_CH_W(CORE, __VA_ARGS__)
#define LOG_C_I(...) LOG_CH_I(CORE, __VA_ARGS__)
#define LOG_C_D(...) LOG_CH_D(CORE, __VA_ARGS__)
// core trace TRACE_LEVEL very verbose and has performance penalty it is not compiled in by default.
#ifdef ADSTLOG_TRACE_LEVEL_CORE_COMPILED_IN
    #define LOG_C_T(...) LOG_CH_T(CORE, __VA_ARGS__)
#else
    #define LOG_C_T(...)
#endif

/**
 * Default logging macros. Shall be used by components and all code logic outside of ADST FW. Logs
 * on actor channel on Actor.name_ module
 */
#define LOG_E(...) LOG_CH_E(ACTOR, __VA_ARGS__)
#define LOG_W(...) LOG_CH_W(ACTOR, __VA_ARGS__)
#define LOG_I(...) LOG_CH_I(ACTOR, __VA_ARGS__)
#define LOG_D(...) LOG_CH_D(ACTOR, __VA_ARGS__)
#define LOG_T(...) LOG_CH_T(ACTOR, __VA_ARGS__)

/**
 * Default logging macros. Shall be used by components and all code logic outside of ADST FW. Logs
 * on actor channel on Actor.name_ module
 */
#define LOG_E_ON(OBJ, ...) LOG_CH_E_ON(OBJ, ACTOR, __VA_ARGS__)
#define LOG_W_ON(OBJ, ...) LOG_CH_W_ON(OBJ, ACTOR, __VA_ARGS__)
#define LOG_I_ON(OBJ, ...) LOG_CH_I_ON(OBJ, ACTOR, __VA_ARGS__)
#define LOG_D_ON(OBJ, ...) LOG_CH_D_ON(OBJ, ACTOR, __VA_ARGS__)
#define LOG_T_ON(OBJ, ...) LOG_CH_T_ON(OBJ, ACTOR, __VA_ARGS__)

// Every Class which works in actor context shall use this trace macro to enable tracing within the class

/**
 * Only defines the member variables for tracing but does not initialize them this might be
 * necessary in case of template classes.
 *
 * If this macro is used, the constructor must call ADSTLOG_INIT_ACTOR_TRACE_MODULES.
 * *Before* considering to use this macro, make sure that ADSTLOG_DEF_DEC_ACTOR_TRACE_MODULES is not
 * suitable.
 */
#define ADSTLOG_DEF_ACTOR_TRACE_MODULES()  \
    ADSTLOG_DEFINE_CH(ACTOR);              \
    ADSTLOG_DEFINE_CH(MSG_TX);             \
    ADSTLOG_DEFINE_CH(MSG_RX);             \
    ADSTLOG_DEFINE_CH(GRPC_RX);            \
    ADSTLOG_DEFINE_CH(GRPC_TX);            \
    ADSTLOG_DEFINE_CH(GRPC);               \
    ADSTLOG_DEFINE_CH(CORE);               \
    ADSTLOG_DEFINE_MODULE(ACTOR, ACTOR);   \
    ADSTLOG_DEFINE_MODULE(MSG_TX, ACTOR);  \
    ADSTLOG_DEFINE_MODULE(MSG_RX, ACTOR);  \
    ADSTLOG_DEFINE_MODULE(GRPC_RX, ACTOR); \
    ADSTLOG_DEFINE_MODULE(GRPC_TX, ACTOR); \
    ADSTLOG_DEFINE_MODULE(GRPC, ACTOR);    \
    ADSTLOG_DEFINE_MODULE(CORE, ACTOR)

/**
 * The macro is meant to used together with ADSTLOG_DEF_ACTOR_TRACE_MODULES. It shall be used in
 * class constructor when ADSTLOG_DEF_ACTOR_TRACE_MODULES was used in the class declaration.
 */
#define ADSTLOG_INIT_ACTOR_TRACE_MODULES(NAME) \
    ADSTLOG_INIT_CH(ACTOR);                    \
    ADSTLOG_INIT_CH(MSG_TX);                   \
    ADSTLOG_INIT_CH(MSG_RX);                   \
    ADSTLOG_INIT_CH(GRPC_RX);                  \
    ADSTLOG_INIT_CH(GRPC_TX);                  \
    ADSTLOG_INIT_CH(GRPC);                     \
    ADSTLOG_INIT_CH(CORE);                     \
    ADSTLOG_INIT_MODULE(ACTOR, ACTOR, NAME);   \
    ADSTLOG_INIT_MODULE(MSG_TX, ACTOR, NAME);  \
    ADSTLOG_INIT_MODULE(MSG_RX, ACTOR, NAME);  \
    ADSTLOG_INIT_MODULE(GRPC_RX, ACTOR, NAME); \
    ADSTLOG_INIT_MODULE(GRPC_TX, ACTOR, NAME); \
    ADSTLOG_INIT_MODULE(GRPC, ACTOR, NAME);    \
    ADSTLOG_INIT_MODULE(CORE, ACTOR, NAME)

/**
 * Register threads used by ADST FW (user shall not create threads).
 */
#define ADSTLOG_REGISTER_THREAD(THREAD_ID, NAME)                     \
    ADSTLOG_CH(ACTOR)->getObj()->Register_Thread(NAME, THREAD_ID);   \
    ADSTLOG_CH(MSG_TX)->getObj()->Register_Thread(NAME, THREAD_ID);  \
    ADSTLOG_CH(MSG_RX)->getObj()->Register_Thread(NAME, THREAD_ID);  \
    ADSTLOG_CH(GRPC_RX)->getObj()->Register_Thread(NAME, THREAD_ID); \
    ADSTLOG_CH(GRPC_TX)->getObj()->Register_Thread(NAME, THREAD_ID); \
    ADSTLOG_CH(GRPC)->getObj()->Register_Thread(NAME, THREAD_ID);    \
    ADSTLOG_CH(CORE)->getObj()->Register_Thread(NAME, THREAD_ID)

#define ADSTLOG_REGISTER_THREAD_ON(OBJ, THREAD_ID, NAME)                 \
    OBJ.ADSTLOG_CH(ACTOR)->getObj()->Register_Thread(NAME, THREAD_ID);   \
    OBJ.ADSTLOG_CH(MSG_TX)->getObj()->Register_Thread(NAME, THREAD_ID);  \
    OBJ.ADSTLOG_CH(MSG_RX)->getObj()->Register_Thread(NAME, THREAD_ID);  \
    OBJ.ADSTLOG_CH(GRPC_RX)->getObj()->Register_Thread(NAME, THREAD_ID); \
    OBJ.ADSTLOG_CH(GRPC_TX)->getObj()->Register_Thread(NAME, THREAD_ID); \
    OBJ.ADSTLOG_CH(GRPC)->getObj()->Register_Thread(NAME, THREAD_ID);    \
    OBJ.ADSTLOG_CH(CORE)->getObj()->Register_Thread(NAME, THREAD_ID)

/**
 * Unregister threads used by ADST FW (user shall not create threads).
 */
#define ADSTLOG_UNREGISTER_THREAD(THREAD_ID)           \
    ADSTLOG_CH(ACTOR)->getObj()->Unregister_Thread(THREAD_ID);   \
    ADSTLOG_CH(MSG_TX)->getObj()->Unregister_Thread(THREAD_ID);  \
    ADSTLOG_CH(MSG_RX)->getObj()->Unregister_Thread(THREAD_ID);  \
    ADSTLOG_CH(GRPC_RX)->getObj()->Unregister_Thread(THREAD_ID); \
    ADSTLOG_CH(GRPC_TX)->getObj()->Unregister_Thread(THREAD_ID); \
    ADSTLOG_CH(GRPC)->getObj()->Unregister_Thread(THREAD_ID);    \
    ADSTLOG_CH(CORE)->getObj()->Unregister_Thread(THREAD_ID)
// clang-format on

#define ADSTLOG_UNREGISTER_THREAD_ON(OBJ, THREAD_ID)                 \
    OBJ.ADSTLOG_CH(ACTOR)->getObj()->Unregister_Thread(THREAD_ID);   \
    OBJ.ADSTLOG_CH(MSG_TX)->getObj()->Unregister_Thread(THREAD_ID);  \
    OBJ.ADSTLOG_CH(MSG_RX)->getObj()->Unregister_Thread(THREAD_ID);  \
    OBJ.ADSTLOG_CH(GRPC_RX)->getObj()->Unregister_Thread(THREAD_ID); \
    OBJ.ADSTLOG_CH(GRPC_TX)->getObj()->Unregister_Thread(THREAD_ID); \
    OBJ.ADSTLOG_CH(GRPC)->getObj()->Unregister_Thread(THREAD_ID);    \
    OBJ.ADSTLOG_CH(CORE)->getObj()->Unregister_Thread(THREAD_ID)
// clang-format on

namespace adst::common {

struct Config;

/**
 * Only one single instance of logger shall exist in any ADST c++ process.
 */
class Logger
{
    // not copyable or movable
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&)                 = delete;
    Logger& operator=(Logger&&) = delete;

public:
    using ClientUPtr  = std::unique_ptr<ManageTraceObj<IP7_Client>>;
    using ChannelUPtr = std::unique_ptr<ManageTraceObj<IP7_Trace>>;

    Logger(const Config& config);
    // there is no flush is needed when the logger is recycled flush is called from the trance Object destructor.
    ~Logger() = default;

private:
    std::string              getClientConfigStr(const Config& config);
    ClientUPtr               client_ = nullptr;
    std::vector<ChannelUPtr> channels_;
};

} // namespace adst::common
