#pragma once
#include <cstdint>
#include <span>
#include <optional>

namespace novac::compilation::object::elf64 {
/** Structural ELF64 validation for native artifact publication. This checks
 * bounds and format coherence, NOT executable semantics or symbol resolution.
 * ET_REL=1 (object), ET_EXEC=2 (executable). */
void verify(std::span<const std::uint8_t> bytes, std::uint16_t expectedType,
            std::optional<std::uint16_t> expectedMachine = {});
} // namespace novac::compilation::object::elf64
