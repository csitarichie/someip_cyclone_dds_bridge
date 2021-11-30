#pragma once

#include <fmt/format.h>
#include <string>
#include "adstutil_cxx/compiler_diagnostics.hpp"
ADST_DISABLE_CLANG_WARNING("unused-parameter")
#include "adstutil_cxx/static_string.hpp"
ADST_RESTORE_CLANG_WARNING()

namespace sstr = ak_toolkit::static_str;

namespace adst::ep::test_engine::core {

/**
 * Stop event which can be used to trigger the shutdown of the dispatcher framework core.
 *
 * When Stop is published core shuts down execution and core.run(..) returns. After receiving the
 * Stop event, actors shall stop publishing new events.
 */
struct Stop
{
    // not copyable or movable
    Stop(const Stop&) = delete;
    Stop(Stop&&)      = delete;

    Stop& operator=(const Stop&) = delete;
    Stop& operator=(Stop&&) = delete;

    Stop() = default;

    static constexpr const auto NAME = sstr::literal("Stop");
};

struct ReqHelper
{
    // not copyable or movable
    ReqHelper(const ReqHelper&) = delete;
    ReqHelper(ReqHelper&&)      = delete;

    ReqHelper& operator=(const ReqHelper&) = delete;
    ReqHelper& operator=(ReqHelper&&) = delete;

    ReqHelper() = default;
};

template <typename ActorT>
struct StartReq : public ReqHelper
{
    static constexpr auto NAME = "StartReq<" + ActorT::NAME + ">";
}; // struct StartReq

struct StartCnfHelper
{
    // not copyable or movable
    StartCnfHelper(const StartCnfHelper&) = delete;
    StartCnfHelper(StartCnfHelper&&)      = delete;

    StartCnfHelper& operator=(const StartCnfHelper&) = delete;
    StartCnfHelper& operator=(StartCnfHelper&&) = delete;

    StartCnfHelper() = default;
};

template <typename ActorT>
struct StartCnf : public StartCnfHelper
{
    static constexpr auto NAME = "StartCnf<" + ActorT::NAME + ">";
}; // struct StartCnf

template <typename ActorT>
struct StopReq : public ReqHelper
{
    static constexpr auto NAME = "StopReq<" + ActorT::NAME + ">";
}; // struct StopReq

struct StopCnfHelper
{
    // not copyable or movable
    StopCnfHelper(const StopCnfHelper&) = delete;
    StopCnfHelper(StartCnfHelper&&)     = delete;

    StopCnfHelper& operator=(const StopCnfHelper&) = delete;
    StopCnfHelper& operator=(StopCnfHelper&&) = delete;

    StopCnfHelper() = default;
};

template <typename ActorT>
struct StopCnf : public StopCnfHelper
{
    static constexpr auto NAME = "StopCnf<" + ActorT::NAME + ">";
}; // struct StopCnf

} // namespace adst::ep::test_engine::core
