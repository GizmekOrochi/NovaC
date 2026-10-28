#include "novac/assets/compilation/backend/x86_64/Elf64Writer.hpp"
#include "novac/assets/compilation/object/Elf64Writer.hpp"

namespace novac::backend::x86_64 {
std::vector<std::uint8_t> makeExecutable(const std::vector<std::uint8_t>& text) {
    return compilation::object::elf64::makeExecutable(text, 62); // EM_X86_64
}
std::vector<std::uint8_t> makeObject(const std::vector<std::uint8_t>& text, std::size_t mainOffset) {
    return compilation::object::elf64::makeObject(text, mainOffset, 62);
}
} // namespace novac::backend::x86_64
