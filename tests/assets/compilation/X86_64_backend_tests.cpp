// Opt-in native backend: no assembler or C compiler participates in code generation.
#include "../../tester.hpp"
#include "novac/assets/compilation/backend/X86_64Backend.hpp"
#include "novac/assets/compilation/backend/x86_64/Codegen.hpp"
#include "novac/assets/compilation/backend/x86_64/Elf64Writer.hpp"
#include "novac/engine/transformation/IR.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <tuple>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(__linux__) && defined(__x86_64__)
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace {
using namespace novac;
using O = ir::Operand;
const ir::TypeRef I32{"i32", "novac.prototype"};

template<class Action> std::string errorOf(Action &&run) {
    try { run(); } catch (const std::runtime_error &err) { return err.what(); }
    return {};
}
ir::ValueId integer(ir::MIRBuilder &b, int value) {
    return b.emitValue("novac.i32.const", {O::fromLiteral(value)}, I32);
}
ir::ValueId operation(ir::MIRBuilder &b, std::string op, ir::ValueId a, ir::ValueId c,
                      ir::TypeRef type = I32) {
    return b.emitValue(std::move(op), {O::fromValue(a), O::fromValue(c)}, type);
}
compilation::CompilationSession compile(ir::MIRModule mir) {
    compilation::CompilationSession session;
    session.artifacts().insert<ir::MIRModule>("mir", std::move(mir));
    backend::BackendRegistry registry;
    registry.add("native", std::make_shared<backend::X86_64Backend>());
    registry.runChecked("native", session);
    return session;
}
const std::vector<std::uint8_t> &executable(const compilation::CompilationSession &session) {
    return session.artifacts().require<std::vector<std::uint8_t>>("x86-64-elf-executable");
}
#if defined(__linux__) && defined(__x86_64__)
int runElf(const std::vector<std::uint8_t> &bytes) {
    char name[] = "/tmp/novac-x64-test-XXXXXX";
    const int file = mkstemp(name);
    if (file < 0) throw std::runtime_error("mkstemp failed");
    if (fchmod(file, 0700) != 0) { close(file); throw std::runtime_error("fchmod failed"); }
    std::size_t cursor = 0;
    while (cursor < bytes.size()) {
        const ssize_t written = write(file, bytes.data()+cursor, bytes.size()-cursor);
        if (written <= 0) { close(file); throw std::runtime_error("write failed"); }
        cursor += static_cast<std::size_t>(written);
    }
    // Linux refuses exec on a file still open for writing (ETXTBSY).
    // Close the creator fd before spawning, and keep the path until waitpid.
    close(file);
    const pid_t pid = fork();
    if (pid < 0) { unlink(name); throw std::runtime_error("fork failed"); }
    if (pid == 0) {
        execl(name,"novac-generated",static_cast<char *>(nullptr));
        _exit(251);
    }
    int status = 0;
    waitpid(pid,&status,0);
    unlink(name);
    if (!WIFEXITED(status)) throw std::runtime_error("generated ELF terminated abnormally");
    return WEXITSTATUS(status);
}
#endif
}

TEST(X86_64Backend, EncodesDirectELFExecutableAndRelocatableObject) {
    ir::MIRBuilder builder;
    const auto v1 = integer(builder,40);
    const auto v2 = integer(builder,2);
    const auto v3 = operation(builder,"novac.i32.add",v1,v2);
    builder.terminate("return", {O::fromValue(v3)});
    auto session = compile(builder.finish());
    const auto &elf = executable(session);
    const auto &obj = session.artifacts().require<std::vector<std::uint8_t>>("x86-64-elf-object");
    const auto &machine = session.artifacts().require<std::vector<std::uint8_t>>("x86-64-machine-code");
    CHECK(elf.size() > 4096);
    CHECK(elf[0] == 0x7f && elf[1] == 'E' && elf[2] == 'L' && elf[3] == 'F');
    CHECK(elf[4] == 2 && elf[5] == 1 && elf[16] == 2 && elf[18] == 62); // ET_EXEC/AMD64
    CHECK(obj[16] == 1 && obj[18] == 62); // ET_REL/AMD64
    CHECK(obj[60] == 5 && obj[62] == 4); // sections + shstrtab
    CHECK(machine[0] == 0xE8 && machine[5] == 0x89); // _start: call main; mov edi,eax
    CHECK(elf[4096] == machine.front());
#if defined(__linux__) && defined(__x86_64__)
    CHECK(runElf(elf) == 42);
#endif
}

TEST(X86_64Backend, SignedIntegerArithmeticAndComparison) {
    for (const auto &[op,left,right,expected] : std::vector<std::tuple<std::string,int,int,int>>{
        {"novac.i32.sub",50,8,42}, {"novac.i32.mul",6,7,42},
        {"novac.i32.lt",-1,0,1}, {"novac.i32.lt",3,-4,0}}) {
        ir::MIRBuilder builder;
        auto l = integer(builder,left), r = integer(builder,right);
        auto v = operation(builder,op,l,r,op == "novac.i32.lt" ? ir::TypeRef{ir::ValueType::Bool} : I32);
        if (op == "novac.i32.lt") { // convert bool to i32 by choosing an output block
            auto zero = integer(builder,0), one = integer(builder,1);
            auto trueBlock = builder.createBlock("true"), falseBlock=builder.createBlock("false");
            builder.terminateWithSuccessors("branch",{O::fromValue(v)}, {{trueBlock,{}},{falseBlock,{}}});
            builder.setCurrentBlock(trueBlock); builder.terminate("return",{O::fromValue(one)});
            builder.setCurrentBlock(falseBlock); builder.terminate("return",{O::fromValue(zero)});
        } else builder.terminate("return",{O::fromValue(v)});
        auto session=compile(builder.finish());
#if defined(__linux__) && defined(__x86_64__)
        CHECK(runElf(executable(session)) == expected);
#else
        CHECK(!executable(session).empty());
#endif
    }
}

TEST(X86_64Backend, BranchesToSameBlockCarryDifferentSSAValues) {
    for (bool selectFirst : {false,true}) {
        ir::MIRBuilder builder;
        auto join = builder.createTypedBlock("join",{I32});
        auto v1=integer(builder,11), v2=integer(builder,42);
        auto cond=builder.emitValue("novac.bool.const",{O::fromLiteral(selectFirst)},ir::ValueType::Bool);
        builder.terminateWithSuccessors("branch",{O::fromValue(cond)},
            {{join,{O::fromValue(v1)}},{join,{O::fromValue(v2)}}});
        builder.setCurrentBlock(join);
        auto module = builder.finish();
        const auto answer = module.blocks[1].arguments[0].value;
        // A terminator is created only after the block parameter is known.
        module.blocks[1].terminator = ir::Terminator{"return", {O::fromValue(answer)}, {}};
        auto session=compile(std::move(module));
#if defined(__linux__) && defined(__x86_64__)
        CHECK(runElf(executable(session)) == (selectFirst ? 11 : 42));
#else
        CHECK(!executable(session).empty());
#endif
    }
}

TEST(X86_64Backend, LoopCarriesSSAValuesAcrossEdges) {
    // Compute 1+2+3+4+5 with real CFG and block parameters.
    ir::MIRBuilder builder;
    auto loop = builder.createTypedBlock("loop", {I32,I32});
    auto body = builder.createTypedBlock("body", {I32,I32});
    auto exit = builder.createTypedBlock("exit", {I32});
    auto zero = integer(builder,0);
    auto five = integer(builder,5);
    auto one = integer(builder,1);
    builder.terminateWithSuccessors("jump",{},{{loop,{O::fromValue(zero),O::fromValue(zero)}}});
    // Block IDs were allocated before parameters; SSA IDs may be interleaved.
    auto current = builder.finish();
    const auto loopI = current.blocks[loop.value].arguments[0].value;
    const auto loopSum = current.blocks[loop.value].arguments[1].value;
    const auto bodyI = current.blocks[body.value].arguments[0].value;
    const auto bodySum = current.blocks[body.value].arguments[1].value;
    const auto exitSum = current.blocks[exit.value].arguments[0].value;
    // Resume construction through a standalone MIR module (same IR contract).
    current.blocks[loop.value].instructions.push_back({"novac.i32.lt",ir::ValueId{500},
        {O::fromValue(loopI), O::fromValue(five)}, ir::ValueType::Bool});
    current.blocks[loop.value].terminator = {"branch",{O::fromValue(ir::ValueId{500})},
        {{body,{O::fromValue(loopI),O::fromValue(loopSum)}},{exit,{O::fromValue(loopSum)}}}};
    current.blocks[body.value].instructions.push_back({"novac.i32.add",ir::ValueId{501},
        {O::fromValue(bodyI),O::fromValue(one)},I32});
    current.blocks[body.value].instructions.push_back({"novac.i32.add",ir::ValueId{502},
        {O::fromValue(bodySum),O::fromValue(ir::ValueId{501})},I32});
    current.blocks[body.value].terminator = {"jump",{},
        {{loop,{O::fromValue(ir::ValueId{501}),O::fromValue(ir::ValueId{502})}}}};
    current.blocks[exit.value].terminator = {"return",{O::fromValue(exitSum)}, {}};
    auto session = compile(std::move(current));
#if defined(__linux__) && defined(__x86_64__)
    CHECK(runElf(executable(session)) == 15);
#else
    CHECK(!executable(session).empty());
#endif
}

TEST(X86_64Backend, ParallelSSATransferPreservesSwapInSelfLoop) {
    ir::MIRBuilder builder;
    auto loop = builder.createTypedBlock("loop", {I32,I32,I32});
    auto exit = builder.createTypedBlock("exit", {I32});
    auto three=integer(builder,3), seven=integer(builder,7), zero=integer(builder,0);
    auto one=integer(builder,1), ten=integer(builder,10);
    builder.terminateWithSuccessors("jump",{},{{loop,{O::fromValue(three),O::fromValue(seven),O::fromValue(zero)}}});
    auto module=builder.finish();
    const auto a=module.blocks[loop.value].arguments[0].value;
    const auto b=module.blocks[loop.value].arguments[1].value;
    const auto n=module.blocks[loop.value].arguments[2].value;
    const auto result=module.blocks[exit.value].arguments[0].value;
    module.blocks[loop.value].instructions.push_back({"novac.i32.lt",ir::ValueId{600},
        {O::fromValue(n),O::fromValue(one)}, ir::ValueType::Bool});
    module.blocks[loop.value].instructions.push_back({"novac.i32.add",ir::ValueId{601},
        {O::fromValue(n),O::fromValue(one)}, I32});
    module.blocks[loop.value].instructions.push_back({"novac.i32.mul",ir::ValueId{602},
        {O::fromValue(a),O::fromValue(ten)}, I32});
    module.blocks[loop.value].instructions.push_back({"novac.i32.add",ir::ValueId{603},
        {O::fromValue(ir::ValueId{602}),O::fromValue(b)}, I32});
    module.blocks[loop.value].terminator={"branch",{O::fromValue(ir::ValueId{600})},
        {{loop,{O::fromValue(b),O::fromValue(a),O::fromValue(ir::ValueId{601})}},
         {exit,{O::fromValue(ir::ValueId{603})}}}};
    module.blocks[exit.value].terminator={"return",{O::fromValue(result)},{}};
    auto session=compile(std::move(module));
#if defined(__linux__) && defined(__x86_64__)
    CHECK(runElf(executable(session)) == 73);
#else
    CHECK(!executable(session).empty());
#endif
}

TEST(X86_64Backend, DoesNotAcceptMalformedOrUnsupportedMIR) {
    {
        ir::MIRBuilder b;
        auto v=b.emitValue("novac.i32.const",{O::fromLiteral(4.5)},I32);
        b.terminate("return",{O::fromValue(v)});
        CHECK(errorOf([&] { compile(b.finish()); }).find("must be int") != std::string::npos);
    }
    {
        ir::MIRBuilder b;
        auto v=b.emitValue("novac.i32.const",{O::fromLiteral(1)},ir::TypeRef{"uint32", "other"});
        b.terminate("return",{O::fromValue(v)});
        CHECK(errorOf([&] { compile(b.finish()); }).find("unsupported SSA type") != std::string::npos);
    }
    {
        ir::MIRBuilder b;
        auto v=b.emitValue("novac.magic",{},I32);
        b.terminate("return",{O::fromValue(v)});
        CHECK(errorOf([&] { compile(b.finish()); }).find("cannot compile MIR operation") != std::string::npos);
    }
    {
        ir::MIRBuilder b;
        auto v=b.emitValue("novac.bool.const",{O::fromLiteral(true)},ir::ValueType::Bool);
        b.terminate("return",{O::fromValue(v)});
        CHECK(errorOf([&] { compile(b.finish()); }).find("operand type mismatch") != std::string::npos);
    }
}

TEST(X86_64Backend, CustomLanguageTypesAreConfiguredNotHardCoded) {
    const ir::TypeRef langI32{"signed-word", "my.language"};
    const ir::TypeRef langBool{"boolean", "my.language"};
    ir::MIRBuilder builder;
    auto a=builder.emitValue("novac.i32.const",{O::fromLiteral(41)},langI32);
    auto b=builder.emitValue("novac.i32.const",{O::fromLiteral(1)},langI32);
    auto sum=builder.emitValue("novac.i32.add",{O::fromValue(a),O::fromValue(b)},langI32);
    builder.terminate("return",{O::fromValue(sum)});
    auto module=builder.finish();
    compilation::CompilationSession session;
    session.artifacts().insert<ir::MIRModule>("mir",module);
    backend::BackendRegistry registry;
    registry.add("native",std::make_shared<backend::X86_64Backend>(
        backend::x86_64::ValueTypes{langI32,langBool}));
    registry.runChecked("native",session);
#if defined(__linux__) && defined(__x86_64__)
    CHECK(runElf(executable(session)) == 42);
#else
    CHECK(!executable(session).empty());
#endif
    // Wrong type mapping must fail without weakening TypeRef identity.
    compilation::CompilationSession wrong;
    wrong.artifacts().insert<ir::MIRModule>("mir",module);
    backend::X86_64Backend defaultBackend;
    CHECK(errorOf([&] { defaultBackend.run(wrong); }).find("unsupported SSA type") != std::string::npos);
}

TEST(X86_64Backend, RejectsConflictingTypeMapsAndOutputCollisions) {
    ir::MIRBuilder builder;
    auto v=integer(builder,42);
    builder.terminate("return",{O::fromValue(v)});
    auto module=builder.finish();
    compilation::CompilationSession session;
    session.artifacts().insert<ir::MIRModule>("mir",module);
    backend::X86_64Backend bad{backend::x86_64::ValueTypes{I32,I32}};
    CHECK(errorOf([&] { bad.run(session); }).find("invalid or ambiguous") != std::string::npos);
    session.artifacts().insert<int>("x86-64-machine-code",1);
    backend::X86_64Backend good;
    CHECK(errorOf([&] { good.run(session); }).find("output artifact already exists") != std::string::npos);
    CHECK(!session.artifacts().has("x86-64-elf-object"));
    CHECK(!session.artifacts().has("x86-64-elf-executable"));
}
