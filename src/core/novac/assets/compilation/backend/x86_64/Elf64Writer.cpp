#include "novac/assets/compilation/backend/x86_64/Elf64Writer.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace novac::backend::x86_64 {
namespace {
class Bytes {
public:
    std::vector<std::uint8_t> data;
    std::size_t size() const noexcept { return data.size(); }
    void u8(std::uint8_t v) { data.push_back(v); }
    void u16(std::uint16_t v) { for(int i=0;i<2;++i) u8(static_cast<std::uint8_t>(v >> (i*8))); }
    void u32(std::uint32_t v) { for(int i=0;i<4;++i) u8(static_cast<std::uint8_t>(v >> (i*8))); }
    void u64(std::uint64_t v) { for(int i=0;i<8;++i) u8(static_cast<std::uint8_t>(v >> (i*8))); }
    void text(const std::string &s) { data.insert(data.end(),s.begin(),s.end()); }
    void append(const std::vector<std::uint8_t> &v) { data.insert(data.end(),v.begin(),v.end()); }
    void align(std::size_t multiple) { while (size()%multiple) u8(0); }
    void zero(std::size_t n) { data.insert(data.end(), n, 0); }
    void header(std::uint16_t type,std::uint64_t entry,std::uint64_t phoff,std::uint64_t shoff,
                std::uint16_t phnum,std::uint16_t shnum,std::uint16_t shstrndx) {
        u8(0x7F); text("ELF"); u8(2); u8(1); u8(1); zero(9); // 64-bit LE, ELF v1
        u16(type); u16(62); u32(1); // EM_X86_64
        u64(entry); u64(phoff); u64(shoff);
        u32(0); u16(64); u16(phnum ? 56 : 0); u16(phnum); u16(shnum ? 64 : 0); u16(shnum); u16(shstrndx);
    }
    void section(std::uint32_t name,std::uint32_t type,std::uint64_t flags,
                 std::uint64_t address,std::uint64_t offset,std::uint64_t bytes,
                 std::uint32_t link,std::uint32_t info,std::uint64_t alignment,std::uint64_t entrySize) {
        u32(name); u32(type); u64(flags); u64(address); u64(offset); u64(bytes);
        u32(link); u32(info); u64(alignment); u64(entrySize);
    }
    void symbol(std::uint32_t name,std::uint8_t info,std::uint16_t shndx,
                std::uint64_t value,std::uint64_t bytes) {
        u32(name); u8(info); u8(0); u16(shndx); u64(value); u64(bytes);
    }
};
void checkedLength(std::size_t n) {
    if (n > (std::numeric_limits<std::uint32_t>::max() - 4096u))
        throw std::runtime_error("ELF64 writer: code image exceeds supported 4GiB limit");
}
} // namespace

std::vector<std::uint8_t> makeExecutable(const std::vector<std::uint8_t> &text) {
    checkedLength(text.size());
    if (text.empty()) throw std::runtime_error("ELF64 writer: no machine code");
    constexpr std::uint64_t loadBase = 0x400000;
    constexpr std::size_t codeOffset = 0x1000;
    const std::uint64_t fileSize = codeOffset + text.size();
    Bytes bytes;
    bytes.header(2, loadBase + codeOffset, 64, 0, 1, 0, 0); // ET_EXEC, entry _start
    // PT_LOAD: map file headers plus code in one read/execute segment.
    bytes.u32(1); bytes.u32(5); bytes.u64(0); bytes.u64(loadBase); bytes.u64(loadBase);
    bytes.u64(fileSize); bytes.u64(fileSize); bytes.u64(0x1000);
    bytes.zero(codeOffset - bytes.size());
    bytes.append(text);
    return std::move(bytes.data);
}

std::vector<std::uint8_t> makeObject(const std::vector<std::uint8_t> &text, std::size_t mainOffset) {
    checkedLength(text.size());
    if (text.empty() || mainOffset >= text.size()) throw std::runtime_error("ELF64 writer: invalid main symbol offset");
    Bytes bytes;
    bytes.header(1, 0, 0, 0, 0, 5, 4); // ET_REL, shoff patched below
    bytes.align(16);
    const auto textOffset = bytes.size();
    bytes.append(text);
    bytes.align(8);
    const auto symtabOffset = bytes.size();
    // Null, local .text section, global _start, global main.
    bytes.zero(24);
    bytes.symbol(0, 3, 1, 0, 0); // STB_LOCAL STT_SECTION
    bytes.symbol(1, 0x12, 1, 0, mainOffset); // _start, STB_GLOBAL STT_FUNC
    bytes.symbol(8, 0x12, 1, mainOffset, text.size()-mainOffset); // main
    const auto symtabSize = bytes.size() - symtabOffset;
    const auto strtabOffset = bytes.size();
    bytes.u8(0); bytes.text("_start"); bytes.u8(0); bytes.text("main"); bytes.u8(0);
    const auto strtabSize = bytes.size() - strtabOffset;
    const auto shstrtabOffset = bytes.size();
    bytes.u8(0); bytes.text(".text"); bytes.u8(0); bytes.text(".symtab"); bytes.u8(0);
    bytes.text(".strtab"); bytes.u8(0); bytes.text(".shstrtab"); bytes.u8(0);
    const auto shstrtabSize = bytes.size() - shstrtabOffset;
    bytes.align(8);
    const auto shoff = bytes.size();
    bytes.zero(64); // SHT_NULL
    bytes.section(1, 1, 6, 0, textOffset, text.size(), 0, 0, 16, 0);
    bytes.section(7, 2, 0, 0, symtabOffset, symtabSize, 3, 2, 8, 24);
    bytes.section(15, 3, 0, 0, strtabOffset, strtabSize, 0, 0, 1, 0);
    bytes.section(23, 3, 0, 0, shstrtabOffset, shstrtabSize, 0, 0, 1, 0);
    // e_shoff is located at byte 40 in ELF64 header.
    for (int i=0;i<8;++i) bytes.data[40+i] = static_cast<std::uint8_t>(shoff >> (i*8));
    return std::move(bytes.data);
}
} // namespace novac::backend::x86_64
