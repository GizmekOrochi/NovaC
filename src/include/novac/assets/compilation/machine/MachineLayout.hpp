#pragma once

#include "novac/engine/transformation/IR.hpp"

#include <cstdint>
#include <unordered_map>
#include <stdexcept>

namespace novac::compilation::machine {

/** Physical value representation, deliberately separate from TypeRef semantics.
 * This M2 backend supports integer scalars of 8, 16, 32 or 64 bits, plus
 * canonical 8-bit booleans. Aggregates/floats need separate layout classes. */
enum class ScalarKind : std::uint8_t { SignedInteger, UnsignedInteger, Boolean };
struct MachineLayout {
    std::uint8_t bits{32};
    ScalarKind kind{ScalarKind::SignedInteger};
    std::uint8_t alignment{4};
    [[nodiscard]] std::uint32_t bytes() const { return bits / 8; }
    void validate() const;
};

/** Each explicit IR type identity gets one, stable machine representation.
 * No guesses based on the TypeRef's spelling ("u16" is not a contract). */
class MachineLayoutRegistry final {
public:
    void add(ir::TypeRef type, MachineLayout layout);
    [[nodiscard]] const MachineLayout &require(const ir::TypeRef &type) const;
    [[nodiscard]] bool has(const ir::TypeRef &type) const noexcept;
    [[nodiscard]] const std::unordered_map<ir::TypeRef, MachineLayout, ir::TypeRefHash> &entries() const noexcept { return entries_; }
private:
    std::unordered_map<ir::TypeRef, MachineLayout, ir::TypeRefHash> entries_{};
};

} // namespace novac::compilation::machine
