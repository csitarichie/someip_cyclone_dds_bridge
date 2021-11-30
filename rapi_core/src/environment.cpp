#include "core/environment.hpp"

using adst::common::ConfigAndLogger;
using adst::ep::test_engine::core::Environment;

Environment::Environment(const ConfigAndLogger& configAndLogger)
    : configAndLogger_(configAndLogger)
{
}
