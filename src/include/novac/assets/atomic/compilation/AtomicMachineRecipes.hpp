#pragma once

#include "novac/assets/compilation/machine/MachineRecipe.hpp"

namespace novac::assets::atomic::compilation {

/** Optional contribution pack. A language can opt in to this new operation
 * without changing the x86-64 backend, or overriding a built-in recipe.
 * All contributions are compiled only in the opt-in compilation prototype. */
void installIncrementI32(novac::compilation::machine::MachineRecipeRegistry &out,
                         const novac::ir::TypeRef &i32);

/** Override only i32 constant ZERO. Other constants use standard recipes. */
void installZeroConstantOverride(novac::compilation::machine::MachineRecipeRegistry &out,
                                 const novac::ir::TypeRef &i32);

} // namespace novac::assets::atomic::compilation
