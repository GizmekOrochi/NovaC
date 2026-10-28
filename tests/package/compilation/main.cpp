#include <novac/assets/compilation/backend/X86_64Backend.hpp>
#include <novac/assets/compilation/target/TargetRegistry.hpp>
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
    // Also exercise the public installed TargetRegistry/NativeBackend API.
    novac::compilation::target::TargetRegistry targets;
    targets.add("linux", novac::backend::makeX86_64LinuxTarget());
    novac::backend::NativeBackend generic{targets.resolveShared(novac::compilation::target::linuxX86_64())};
    novac::compilation::CompilationSession session2;
    auto second=novac::ir::MIRBuilder{};
    auto v=second.emitValue("novac.i32.const",{novac::ir::Operand::fromLiteral(42)},i32);
    second.terminate("return",{novac::ir::Operand::fromValue(v)});
    session2.artifacts().insert<novac::ir::MIRModule>("mir",second.finish());
    generic.run(session2);
    return (obj.size()>64 && elf.size()>4096 && elf[0]==0x7f && elf[1]=='E' &&
            session2.artifacts().has("x86-64-elf-object"))?0:1;
}
