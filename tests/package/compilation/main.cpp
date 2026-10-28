#include <novac/assets/compilation/backend/X86_64Backend.hpp>
#include <novac/assets/compilation/Compilation.hpp>
#include <novac/engine/transformation/IR.hpp>
#include <cstdint>
#include <vector>
int main() {
    novac::ir::MIRBuilder builder;
    const novac::ir::TypeRef i32{"i32","novac.prototype"};
    const auto result=builder.emitValue("novac.i32.const",
        {novac::ir::Operand::fromLiteral(42)},i32);
    builder.terminate("return",{novac::ir::Operand::fromValue(result)});
    novac::compilation::CompilationSession session;
    session.artifacts().insert<novac::ir::MIRModule>("mir",builder.finish());
    novac::backend::X86_64Backend backend;
    backend.run(session);
    const auto &obj=session.artifacts().require<std::vector<std::uint8_t>>("x86-64-elf-object");
    const auto &elf=session.artifacts().require<std::vector<std::uint8_t>>("x86-64-elf-executable");
    return (obj.size()>64 && elf.size()>4096 && elf[0]==0x7f && elf[1]=='E')?0:1;
}
