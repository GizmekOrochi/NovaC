#pragma once

#include "novac/assets/types/IRTypeAdapter.hpp"
#include "novac/assets/compilation/machine/MachineRecipe.hpp"
#include "novac/assets/compilation/machine/MachineLayout.hpp"

namespace novac::assets::types::compilation {

/** Bind a recipe to the canonical TypeId (so aliases share an IR identity).
 * This only binds identities: it does NOT assert that the physical layout of
 * the type is supported by a backend. The backend still verifies its types. */
/** Register the verified physical representation of a canonical Types primitive.
 * No automatic inheritance of arithmetic semantics is inferred from size. */
void addPrimitiveLayout(novac::compilation::machine::MachineLayoutRegistry &out,
                        IRTypeAdapter &typeAdapter, const TypeController &types, TypeId type);

void addTypedRecipe(novac::compilation::machine::MachineRecipeRegistry &out,
                    IRTypeAdapter &typeAdapter, TypeId type,
                    novac::compilation::machine::MachineRecipe recipe);

} // namespace novac::assets::types::compilation
