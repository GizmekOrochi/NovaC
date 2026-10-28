#pragma once
#include "novac/assets/compilation/machine/MachineRecipe.hpp"
#include <optional>

namespace novac::compilation {
/** Session-scoped preferences. Unset means retain the target provider's
 * configured default. This object is copied, never mutates a shared target. */
struct CompilationOptions {
    std::optional<machine::OptimizationGoal> optimization{};
};
} // namespace novac::compilation
