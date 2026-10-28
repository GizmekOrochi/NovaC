#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace novac::backend::x86_64 {
/** Linux x86-64 ET_EXEC, no external toolchain or runtime. */
std::vector<std::uint8_t> makeExecutable(const std::vector<std::uint8_t> &text);
/** ELF64 ET_REL with .text, .symtab, .strtab, .shstrtab and global _start/main.
 * Local control-flow references are fully resolved: no external relocations. */
std::vector<std::uint8_t> makeObject(const std::vector<std::uint8_t> &text, std::size_t mainOffset);
} // namespace novac::backend::x86_64
