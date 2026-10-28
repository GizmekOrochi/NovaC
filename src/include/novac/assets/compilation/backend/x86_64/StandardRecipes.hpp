#pragma once
#include "novac/assets/compilation/machine/MachineRecipe.hpp"
#include "novac/assets/compilation/backend/x86_64/Codegen.hpp"

namespace novac::backend::x86_64 {
/** Baseline recipes. Third-party asset packs can override any operation with
 * a more specific or higher-tier recipe without editing Codegen.cpp. */
compilation::machine::MachineRecipeRegistry standardRecipes(const ValueTypes &types);
/** Install standard scalar integer operations for an explicitly typed identity.
 * The language owns the semantics of these op names and must choose whether
 * overflow wraps, traps or has any other meaning. These recipes wrap modulo 2^N. */
void installScalarRecipes(compilation::machine::MachineRecipeRegistry &registry,
                          const ir::TypeRef &type, compilation::machine::MachineLayout layout,
                          const std::string &prefix,
                          ir::TypeRef booleanType = ir::ValueType::Bool);
} // namespace novac::backend::x86_64
