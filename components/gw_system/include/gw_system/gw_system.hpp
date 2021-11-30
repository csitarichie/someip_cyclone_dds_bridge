#pragma once
#include "core/core.hpp"

using adst::ep::test_engine::core::Actor;
using adst::ep::test_engine::core::Environment;
using adst::ep::test_engine::core::Stop;

struct GwSystemRoot : public Actor
{

  // not copyable or movable
  GwSystemRoot(const GwSystemRoot&) = delete;
  GwSystemRoot(GwSystemRoot&&)      = delete;

  GwSystemRoot& operator=(const GwSystemRoot&) = delete;
  GwSystemRoot& operator=(GwSystemRoot&&) = delete;

  explicit GwSystemRoot(const Environment& env)
      : Actor(NAME.c_str(), env)
  {

  }

  ~GwSystemRoot() override    = default;
  static constexpr const auto NAME = sstr::literal("GwSystemRoot");

};
