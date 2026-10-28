#include "novac/assets/compilation/machine/MachineLayout.hpp"

namespace novac::compilation::machine {
void MachineLayout::validate() const {
    if (bits != 8 && bits != 16 && bits != 32 && bits != 64)
        throw std::runtime_error("MachineLayout: supported scalar widths are 8, 16, 32, 64 bits");
    if (kind == ScalarKind::Boolean && bits != 8)
        throw std::runtime_error("MachineLayout: booleans require canonical 8-bit representation");
    if (alignment == 0 || alignment > 8 || (alignment & (alignment-1)) != 0)
        throw std::runtime_error("MachineLayout: alignment must be a power of two between 1 and 8 bytes");
    if (alignment > bytes())
        throw std::runtime_error("MachineLayout: over-aligned scalar not supported by the M2 stack allocator");
}
void MachineLayoutRegistry::add(ir::TypeRef type, MachineLayout layout) {
    if (!type.valid() || type.isVoid() || type.unresolved())
        throw std::runtime_error("MachineLayout: invalid/unresolved type reference");
    layout.validate();
    if (!entries_.emplace(type, layout).second)
        throw std::runtime_error("MachineLayout: duplicate type identity '" + type.display() + "'");
}
const MachineLayout &MachineLayoutRegistry::require(const ir::TypeRef &type) const {
    const auto it = entries_.find(type);
    if (it == entries_.end())
        throw std::runtime_error("MachineLayout: no machine representation for '" + type.display() + "'");
    return it->second;
}
bool MachineLayoutRegistry::has(const ir::TypeRef &type) const noexcept { return entries_.contains(type); }
} // namespace novac::compilation::machine
