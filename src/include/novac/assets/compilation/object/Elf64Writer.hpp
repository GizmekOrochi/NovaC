#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace novac::compilation::object::elf64 {
/** Minimal Linux-oriented ELF64 writer accepting a target ELF e_machine.
 * This module writes container metadata only: native targets are responsible
 * for instruction encoding, entry point semantics, ABI and relocations. */
std::vector<std::uint8_t> makeExecutable(const std::vector<std::uint8_t>& text,
                                         std::uint16_t machine);
std::vector<std::uint8_t> makeObject(const std::vector<std::uint8_t>& text,
                                     std::size_t mainOffset,
                                     std::uint16_t machine);
} // namespace novac::compilation::object::elf64
