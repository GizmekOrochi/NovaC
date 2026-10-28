#pragma once
#include "novac/engine/transformation/IR.hpp"
#include "novac/assets/compilation/machine/MachineRecipe.hpp"
#include "novac/assets/compilation/machine/MachineLayout.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace novac::backend::x86_64 {

/** A language must explicitly identify which of its types have the target
 * signed 32-bit and boolean representations. The default is the M0 demo's
 * identity; independent languages should pass their own TypeRefs. */
struct ValueTypes {
    ir::TypeRef i32{"i32", "novac.prototype"};
    ir::TypeRef boolean{ir::ValueType::Bool};
    /** Additional machine types, explicitly associated with IR identities. */
    compilation::machine::MachineLayoutRegistry layouts{};
};

/** Native code image: _start (Linux syscall exit) followed by a System V main.
 * All displacements to local blocks and the call to main are resolved here. */
struct MachineImage {
    std::vector<std::uint8_t> text;
    std::size_t mainOffset{};
};

/** Stack-slot baseline: conservative, correct SSA edge transfers, no register allocator yet. */
MachineImage selectAndEncode(const ir::MIRModule &module, const ValueTypes &types = {});
MachineImage selectAndEncode(const ir::MIRModule &module, const ValueTypes &types,
                             const compilation::machine::MachineRecipeRegistry &recipes,
                             const compilation::machine::MachineTarget &target);

} // namespace novac::backend::x86_64
