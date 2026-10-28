#include "../../tester.hpp"
#include "novac/assets/compilation/target/TargetRegistry.hpp"
#include "novac/assets/compilation/backend/X86_64Backend.hpp"
#include "novac/assets/compilation/backend/NativeBackend.hpp"
#include "novac/assets/compilation/backend/Backend.hpp"
#include "novac/assets/compilation/machine/NativeFramePlan.hpp"
#include "novac/assets/compilation/object/Elf64Writer.hpp"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace novac;
using namespace novac::compilation;
using namespace novac::backend;
using namespace novac::compilation::target;
const ir::TypeRef I32{"i32","novac.prototype"};
template<class F> std::string error(F&& f) {
    try { f(); } catch (const std::runtime_error& e) { return e.what(); }
    return {};
}
class DummyTarget final : public NativeTarget {
public:
    explicit DummyTarget(TargetSpec spec = {"dummy","test-os","dummy-abi","dummy-object","generic",{}})
        : spec_{std::move(spec)} {}
    const TargetSpec& specification() const noexcept override { return spec_; }
    BackendCapabilities capabilities() const override {
        return {"mir","dummy-native-output",{"novac.i32.const"},{"return"}};
    }
    void preflight(const ir::MIRModule&) const override {}
    NativeBuild generate(const ir::MIRModule&) const override {
        return {{{"dummy-native-output",{0x42,0x01}}}};
    }
private:
    TargetSpec spec_;
};
}

TEST(TargetRegistry, RejectsInvalidDuplicateAndUnsupportedPlatform) {
    TargetRegistry registry;
    CHECK(error([&]{ registry.add("invalid", nullptr); }).find("required")!=std::string::npos);
    registry.add("linux-x86",makeX86_64LinuxTarget());
    CHECK(registry.has("linux-x86"));
    CHECK(registry.require("linux-x86").specification()==linuxX86_64());
    CHECK(registry.resolve(linuxX86_64()).specification()==linuxX86_64());
    registry.add("x86-again",makeX86_64LinuxTarget());
    CHECK(registry.providersFor(linuxX86_64()) == (std::vector<std::string>{"linux-x86", "x86-again"}));
    CHECK(error([&]{ (void)registry.resolve(linuxX86_64()); }).find("ambiguous target")!=std::string::npos);
    CHECK(registry.resolveShared(linuxX86_64(), "x86-again")->specification()==linuxX86_64());
    CHECK(error([&]{ registry.add("x86-again",makeX86_64LinuxTarget()); }).find("duplicate")!=std::string::npos);
    auto mac = linuxX86_64();
    mac.operatingSystem="macos";mac.objectFormat="mach-o";mac.abi="apple";
    CHECK(error([&]{ (void)registry.resolve(mac); }).find("unsupported target")!=std::string::npos);
    auto aarch64 = linuxX86_64(); aarch64.architecture="aarch64";
    CHECK(error([&]{ (void)registry.resolve(aarch64); }).find("unsupported target")!=std::string::npos);
    auto cpu = linuxX86_64(); cpu.cpu="apple-m2";
    CHECK(!error([&]{ (void)registry.resolve(cpu); }).empty());
    CHECK(registry.available().size()==2);
    registry.freeze();
    CHECK(error([&]{ registry.add("dummy",std::make_shared<DummyTarget>()); }).find("frozen")!=std::string::npos);
}

TEST(TargetRegistry, CustomArchitectureCompilesWithoutChangingControllerOrEncoder) {
    TargetRegistry targets;
    auto dummy=std::make_shared<DummyTarget>();
    targets.add("toy",dummy);
    auto selected=targets.resolveShared(dummy->specification());
    BackendRegistry backends;
    backends.add("toy-native",std::make_shared<NativeBackend>(selected));
    ir::MIRBuilder builder;
    const auto c=builder.emitValue("novac.i32.const",{ir::Operand::fromLiteral(42)},I32);
    builder.terminate("return",{ir::Operand::fromValue(c)});
    compilation::CompilationSession session;
    session.artifacts().insert<ir::MIRModule>("mir",builder.finish());
    backends.runChecked("toy-native",session);
    const auto& bytes=session.artifacts().require<std::vector<std::uint8_t>>("dummy-native-output");
    CHECK(bytes.size()==2 && bytes.front()==0x42);
    CHECK(!session.artifacts().has("x86-64-elf-executable"));
}

TEST(TargetRegistry, NativeBackendRejectsExistingOutputsBeforePublishing) {
    NativeBackend native{std::make_shared<DummyTarget>()};
    ir::MIRBuilder builder;
    const auto c=builder.emitValue("novac.i32.const",{ir::Operand::fromLiteral(42)},I32);
    builder.terminate("return",{ir::Operand::fromValue(c)});
    compilation::CompilationSession session;
    session.artifacts().insert<ir::MIRModule>("mir",builder.finish());
    session.artifacts().insert<std::vector<std::uint8_t>>("dummy-native-output",{11});
    CHECK(error([&]{ native.run(session); }).find("already exists")!=std::string::npos);
    CHECK(session.artifacts().require<std::vector<std::uint8_t>>("dummy-native-output").front()==11);
}

TEST(NativeFramePlan, SharedSSASlotCalculationIndependentOfArchitecture) {
    ir::MIRBuilder builder;
    const auto argBlock=builder.createTypedBlock("successor",{I32});
    const auto value=builder.emitValue("novac.i32.const",{ir::Operand::fromLiteral(3)},I32);
    builder.terminateWithSuccessors("jump",{},{{argBlock,{ir::Operand::fromValue(value)}}});
    builder.setCurrentBlock(argBlock);
    auto mir=builder.finish();
    const auto arg=mir.blocks[argBlock.value].arguments[0].value;
    mir.blocks[argBlock.value].terminator=ir::Terminator{"return",{ir::Operand::fromValue(arg)}, {}};
    machine::MachineLayoutRegistry layouts;
    layouts.add(I32,{32,machine::ScalarKind::SignedInteger,4});
    machine::NativeFramePlan plan{mir,std::move(layouts)};
    CHECK(plan.require(value).offset>0);
    CHECK(plan.require(arg).offset>0);
    CHECK(plan.require(value).offset!=plan.require(arg).offset);
    CHECK(plan.maxEdgeArguments()==1);
    CHECK(plan.frameBytes()%16==0);
    CHECK(plan.frameBytes()>=plan.scratchStart()+8);
}

TEST(TargetRegistry, GenericELFWriterHonorsTargetEMachineWithoutEncodingInstructions) {
    const std::vector<std::uint8_t> fakeCode{0x00,0x01,0x02,0x03};
    const auto obj=compilation::object::elf64::makeObject(fakeCode,1,183);
    CHECK(obj[18]==183 && obj[19]==0); // ELF e_machine metadata only
    const auto executable=compilation::object::elf64::makeExecutable(fakeCode,183);
    CHECK(executable[18]==183 && executable[19]==0);
    CHECK(executable[4096]==fakeCode[0]);
}

namespace {
/** A downstream extension that inherits the working x86-64 target instead of
 * copying its ELF writer, instruction selector or Linux entry-point code. */
class InheritedX86Target final : public LinuxX86_64Target {
public:
    using LinuxX86_64Target::LinuxX86_64Target;
    mutable std::vector<machine::OptimizationGoal> goals;
protected:
    NativeBuild generateForTarget(const ir::MIRModule &mir,
                                  const machine::MachineTarget &target) const override {
        goals.push_back(target.goal);
        NativeBuild result = LinuxX86_64Target::generateForTarget(mir, target);
        result.artifacts.push_back({"extension-metadata", {0xAB, 0x42}});
        return result;
    }
};
}

TEST(TargetRegistry, X86TargetInheritanceReusesNativeCodegenAndAddsArtifact) {
    TargetRegistry registry;
    registry.add("extended-x86", std::make_shared<InheritedX86Target>());
    auto provider = registry.resolveShared(linuxX86_64());
    BackendRegistry backends;
    backends.add("extended-x86", std::make_shared<NativeBackend>(provider));
    ir::MIRBuilder builder;
    const auto c = builder.emitValue("novac.i32.const", {ir::Operand::fromLiteral(42)}, I32);
    builder.terminate("return", {ir::Operand::fromValue(c)});
    CompilationSession session;
    session.artifacts().insert<ir::MIRModule>("mir", builder.finish());
    backends.runChecked("extended-x86", session);
    const auto &code = session.artifacts().require<std::vector<std::uint8_t>>("x86-64-machine-code");
    const auto &elf = session.artifacts().require<std::vector<std::uint8_t>>("x86-64-elf-executable");
    const auto &extra = session.artifacts().require<std::vector<std::uint8_t>>("extension-metadata");
    CHECK(code.size() > 5);
    CHECK(elf.size() > code.size());
    CHECK(elf[0] == 0x7f && elf[1] == 'E' && elf[2] == 'L' && elf[3] == 'F');
    const auto *descriptor=session.artifacts().describe("x86-64-elf-executable");
    CHECK(descriptor != nullptr);
    CHECK(descriptor->kind == "executable");
    CHECK(descriptor->target.find("x86-64") != std::string::npos);
    CHECK(extra.size() == 2 && extra[0] == 0xAB && extra[1] == 0x42);
}

TEST(TargetRegistry, SupportsMultipleDerivedProvidersAndExplicitSelection) {
    TargetRegistry registry;
    registry.add("standard", makeX86_64LinuxTarget());
    registry.add("derived", std::make_shared<InheritedX86Target>());
    CHECK(error([&]{ (void)registry.resolve(linuxX86_64()); }).find("standard") != std::string::npos);
    CHECK(error([&]{ (void)registry.resolve(linuxX86_64()); }).find("derived") != std::string::npos);
    auto selected = registry.resolveShared(linuxX86_64(), "derived");
    CHECK(dynamic_cast<const InheritedX86Target*>(selected.get()) != nullptr);
    auto other = std::make_shared<DummyTarget>();
    registry.add("toy", other);
    CHECK(error([&]{ (void)registry.resolveShared(linuxX86_64(), "toy"); }).find("does not support") != std::string::npos);
    CHECK(error([&]{ (void)registry.resolveShared(linuxX86_64(), "absent"); }).find("unknown target") != std::string::npos);
}

TEST(NativeBackend, BatchPublicationIsAtomicAndPreservesPreexistingValues) {
    CompilationSession session;
    session.artifacts().insert<int>("keep", 17);
    CHECK(error([&]{ session.artifacts().insertByteBatch({{"ok",{1}}, {"keep",{2}}}); }).find("duplicate")!=std::string::npos);
    CHECK(!session.artifacts().has("ok"));
    CHECK(session.artifacts().require<int>("keep") == 17);
    CHECK(error([&]{ session.artifacts().insertByteBatch({{"duplicate",{1}}, {"duplicate",{2}}}); }).find("duplicate")!=std::string::npos);
    CHECK(!session.artifacts().has("duplicate"));
    session.artifacts().insertByteBatch({{"one",{1}}, {"two",{2}}});
    CHECK(session.artifacts().require<std::vector<std::uint8_t>>("one")[0] == 1);
    CHECK(session.artifacts().require<std::vector<std::uint8_t>>("two")[0] == 2);
    CHECK(session.artifacts().describe("one") == nullptr);
    CHECK(error([&]{ session.artifacts().insertByteBatch({{"three",{3}}},{{"kind", "one"},{"extra", "two"}}); }).find("descriptor count")!=std::string::npos);
    CHECK(!session.artifacts().has("three"));
}

namespace {
class CountingTarget final : public NativeTarget {
public:
    const TargetSpec& specification() const noexcept override { return spec_; }
    BackendCapabilities capabilities() const override { return {"mir","counted",{"novac.i32.const"},{"return"}}; }
    void preflight(const ir::MIRModule &) const override { ++preflights; }
    NativeBuild generate(const ir::MIRModule &) const override { return {{{"counted",{1}}}}; }
    mutable int preflights{0};
private:
    TargetSpec spec_{"counted","test-os","test-abi","binary","generic",{}};
};
}
TEST(NativeBackend, PreflightOccursOnceInCheckedRunnerAndDirectCallsStaySafe) {
    auto target = std::make_shared<CountingTarget>();
    BackendRegistry backends;
    backends.add("count", std::make_shared<NativeBackend>(target));
    ir::MIRBuilder builder;
    auto v = builder.emitValue("novac.i32.const", {ir::Operand::fromLiteral(42)}, I32);
    builder.terminate("return", {ir::Operand::fromValue(v)});
    auto mir = builder.finish();
    CompilationSession checked;
    checked.artifacts().insert<ir::MIRModule>("mir", mir);
    backends.runChecked("count", checked);
    CHECK(target->preflights == 1);
    CompilationSession direct;
    direct.artifacts().insert<ir::MIRModule>("mir", std::move(mir));
    backends.run("count", direct);
    CHECK(target->preflights == 2);
}

TEST(TargetRegistry, DerivedTargetPreservesSessionOptionsAndCustomArtifacts) {
    auto target = std::make_shared<InheritedX86Target>();
    ir::MIRBuilder builder;
    const auto v=builder.emitValue("novac.i32.const",{ir::Operand::fromLiteral(42)},I32);
    builder.terminate("return",{ir::Operand::fromValue(v)});
    const auto program = builder.finish();
    for (auto goal : {machine::OptimizationGoal::Size, machine::OptimizationGoal::Debug}) {
        NativeBackend backend{target, CompilationOptions{goal}};
        CompilationSession session;
        session.artifacts().insert<ir::MIRModule>("mir",program);
        backend.run(session);
        CHECK(session.artifacts().has("extension-metadata"));
        CHECK(session.artifacts().require<std::vector<std::uint8_t>>("extension-metadata")[0] == 0xAB);
        CHECK(session.artifacts().has("x86-64-elf-executable"));
        CHECK(target->goals.back() == goal);
    }
    CHECK(target->goals.size() == 2);
}

TEST(NativeBackend, DirectAndPrevalidatedEntryPointsRejectUnterminatedMIR) {
    auto target = std::make_shared<CountingTarget>(); // preflight deliberately does nothing
    NativeBackend native{target};
    ir::MIRBuilder builder;
    builder.emitValue("novac.i32.const", {ir::Operand::fromLiteral(42)}, I32);
    auto partial = builder.finish();
    for (bool claimedValidated : {false, true}) {
        CompilationSession session;
        session.artifacts().insert<ir::MIRModule>("mir", partial);
        auto message = error([&] {
            if (claimedValidated) native.runPrevalidated(session);
            else native.run(session);
        });
        CHECK(message.find("terminator") != std::string::npos);
        CHECK(!session.artifacts().has("counted"));
    }
    BackendRegistry registry;
    registry.add("count", std::make_shared<NativeBackend>(target));
    CompilationSession session;
    session.artifacts().insert<ir::MIRModule>("mir", partial);
    CHECK(error([&] { registry.runChecked("count", session); }).find("terminator") != std::string::npos);
    CHECK(!session.artifacts().has("counted"));
}

namespace {
class MalformedELFTarget final : public NativeTarget {
public:
    explicit MalformedELFTarget(std::vector<std::uint8_t> bytes, NativeArtifactKind kind)
        : bytes_{std::move(bytes)}, kind_{kind} {}
    const TargetSpec& specification() const noexcept override { return spec_; }
    BackendCapabilities capabilities() const override { return {"mir", "malformed-output", {"novac.i32.const"}, {"return"}}; }
    void preflight(const ir::MIRModule&) const override {}
    NativeBuild generate(const ir::MIRModule&) const override {
        return {{{"malformed-output", bytes_, kind_, spec_}}};
    }
private:
    std::vector<std::uint8_t> bytes_;
    NativeArtifactKind kind_;
    TargetSpec spec_{linuxX86_64()};
};
}
TEST(NativeBackend, RejectsTruncatedFakeELFAndNonLoadableExecutable) {
    ir::MIRBuilder builder;
    const auto v=builder.emitValue("novac.i32.const", {ir::Operand::fromLiteral(42)}, I32);
    builder.terminate("return", {ir::Operand::fromValue(v)});
    auto program = builder.finish();
    auto checkReject = [&](const std::vector<std::uint8_t>& bytes, NativeArtifactKind kind) {
        NativeBackend native{std::make_shared<MalformedELFTarget>(bytes, kind)};
        CompilationSession session;
        session.artifacts().insert<ir::MIRModule>("mir", program);
        CHECK(!error([&] { native.run(session); }).empty());
        CHECK(!session.artifacts().has("malformed-output"));
    };
    // This fake header previously passed: first 20 bytes look like ELF64/ET_EXEC/x86.
    std::vector<std::uint8_t> fake(20, 0);
    fake[0]=0x7f; fake[1]='E'; fake[2]='L'; fake[3]='F'; fake[4]=2; fake[5]=1;
    fake[16]=2; fake[18]=62;
    checkReject(fake, NativeArtifactKind::Executable);
    auto valid = compilation::object::elf64::makeExecutable({0xC3},62);
    checkReject({valid.begin(), valid.begin()+75}, NativeArtifactKind::Executable);
    valid[24]=0; valid[25]=0; valid[26]=0; // erase e_entry
    checkReject(valid, NativeArtifactKind::Executable);
    auto object = compilation::object::elf64::makeObject({0xC3},0,62);
    object[40]=0; object[41]=0; // e_shoff -> 0, not a valid section table
    checkReject(object, NativeArtifactKind::ObjectFile);
}

TEST(NativeBackend, ELFArtifactsFromRealTargetRemainValidAndTyped) {
    ir::MIRBuilder builder;
    const auto v=builder.emitValue("novac.i32.const", {ir::Operand::fromLiteral(42)}, I32);
    builder.terminate("return", {ir::Operand::fromValue(v)});
    NativeBackend backend{makeX86_64LinuxTarget()};
    CompilationSession session;
    session.artifacts().insert<ir::MIRModule>("mir", builder.finish());
    backend.run(session);
    CHECK(session.artifacts().has("x86-64-elf-object"));
    CHECK(session.artifacts().has("x86-64-elf-executable"));
    CHECK(session.artifacts().describe("x86-64-elf-object")->kind == "object-file");
    CHECK(session.artifacts().describe("x86-64-elf-executable")->kind == "executable");
}

namespace {
class MistypedTarget final : public NativeTarget {
public:
    const TargetSpec& specification() const noexcept override { return spec_; }
    BackendCapabilities capabilities() const override {
        BackendCapabilities cap{"mir", "out", {"novac.i32.const"}, {"return"}};
        cap.outputKind=NativeArtifactKind::Executable;
        return cap;
    }
    void preflight(const ir::MIRModule&) const override {}
    NativeBuild generate(const ir::MIRModule&) const override {
        return {{{"out", {42}, NativeArtifactKind::Opaque}}};
    }
private:
    TargetSpec spec_{"counted", "test-os", "test-abi", "binary", "generic", {}};
};
}
TEST(NativeBackend, TypedOutputContractRejectsWrongArtifactKind) {
    ir::MIRBuilder builder;
    const auto value=builder.emitValue("novac.i32.const", {ir::Operand::fromLiteral(42)}, I32);
    builder.terminate("return", {ir::Operand::fromValue(value)});
    NativeBackend native{std::make_shared<MistypedTarget>()};
    CompilationSession session;
    session.artifacts().insert<ir::MIRModule>("mir", builder.finish());
    CHECK(error([&] { native.run(session); }).find("wrong artifact kind") != std::string::npos);
    CHECK(!session.artifacts().has("out"));
}
