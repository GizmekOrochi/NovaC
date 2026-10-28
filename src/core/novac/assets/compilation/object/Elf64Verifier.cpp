#include "novac/assets/compilation/object/Elf64Verifier.hpp"
#include <stdexcept>
#include <string>

namespace novac::compilation::object::elf64 {
namespace {
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::uint64_t read(std::size_t offset, std::size_t size) const {
        if (offset > bytes.size() || size > bytes.size() - offset)
            throw std::runtime_error("ELF64 verifier: truncated header or table");
        std::uint64_t result = 0;
        for (std::size_t i = 0; i < size; ++i)
            result |= std::uint64_t(bytes[offset+i]) << (8*i);
        return result;
    }
    void bounds(std::uint64_t offset, std::uint64_t length) const {
        if (offset > bytes.size() || length > bytes.size()-offset)
            throw std::runtime_error("ELF64 verifier: table or segment outside artifact");
    }
    void table(std::uint64_t offset, std::uint64_t count, std::uint64_t stride) const {
        if (count == 0) return;
        if (offset == 0 || stride == 0 || offset > bytes.size() || count > (bytes.size()-offset)/stride)
            throw std::runtime_error("ELF64 verifier: truncated or invalid table");
    }
};
}
void verify(std::span<const std::uint8_t> bytes, std::uint16_t expectedType,
            std::optional<std::uint16_t> expectedMachine) {
    Reader b{bytes};
    b.bounds(0, 64);
    if (b.read(0,1)!=0x7f || b.read(1,1)!='E' || b.read(2,1)!='L' || b.read(3,1)!='F' ||
        b.read(4,1)!=2 || b.read(5,1)!=1 || b.read(6,1)!=1 || b.read(7,1)!=0 ||
        b.read(16,2)!=expectedType || b.read(20,4)!=1 || b.read(52,2)!=64)
        throw std::runtime_error("ELF64 verifier: invalid identification, header, or file kind");
    if (expectedMachine && b.read(18,2)!=*expectedMachine)
        throw std::runtime_error("ELF64 verifier: wrong machine architecture");
    const auto phoff=b.read(32,8), shoff=b.read(40,8);
    const auto phentsize=b.read(54,2), phnum=b.read(56,2);
    const auto shentsize=b.read(58,2), shnum=b.read(60,2), shstrndx=b.read(62,2);
    if ((phnum && phentsize!=56) || (!phnum && phoff!=0) ||
        (shnum && shentsize!=64) || (!shnum && shoff!=0) ||
        (shstrndx!=0 && shstrndx>=shnum) ||
        (phnum && phoff<64) || (shnum && shoff<64))
        throw std::runtime_error("ELF64 verifier: invalid table layout");
    b.table(phoff, phnum, phentsize);
    b.table(shoff, shnum, shentsize);
    bool loadsEntry=false, hasLoad=false;
    const auto entry=b.read(24,8);
    for (std::size_t i=0;i<phnum;++i) {
        const auto pos=phoff+i*56;
        const auto kind=b.read(pos,4), flags=b.read(pos+4,4);
        const auto offset=b.read(pos+8,8), vaddr=b.read(pos+16,8);
        const auto fileSize=b.read(pos+32,8), memSize=b.read(pos+40,8);
        b.bounds(offset,fileSize);
        if (fileSize>memSize) throw std::runtime_error("ELF64 verifier: invalid segment sizes");
        if (kind==1) { // PT_LOAD
            hasLoad=true;
            if (entry>=vaddr && entry-vaddr<fileSize && (flags&1)) loadsEntry=true;
        }
    }
    for (std::size_t i=0;i<shnum;++i) {
        const auto pos=shoff+i*64;
        const auto kind=b.read(pos+4,4);
        const auto offset=b.read(pos+24,8), size=b.read(pos+32,8);
        if (kind!=8) b.bounds(offset,size); // SHT_NOBITS has no file payload
        if (kind==2 || kind==11) { // symbol tables must link to a section
            const auto link=b.read(pos+40,4), entsize=b.read(pos+56,8);
            if (link>=shnum || entsize!=24 || size%entsize!=0)
                throw std::runtime_error("ELF64 verifier: invalid symbol table");
        }
    }
    if (expectedType==2 && (!hasLoad || !loadsEntry || phnum==0 || !entry))
        throw std::runtime_error("ELF64 verifier: executable has no loadable entry point");
    if (expectedType==1 && (shnum==0 || shstrndx==0))
        throw std::runtime_error("ELF64 verifier: relocatable object has no section table");
}
} // namespace novac::compilation::object::elf64
