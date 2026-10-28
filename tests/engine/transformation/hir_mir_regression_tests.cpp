#include "../../tester.hpp"
#include "novac/engine/transformation/IR.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace novac::ir;
using Result = MIRLoweringResult;
using Input = MIRLoweringInput;
using Ctx = MIRLoweringContext;
using Registry = LoweringRegistry;

template <typename Function>
std::string failure(Function &&function) {
    try { function(); } catch (const std::runtime_error &error) { return error.what(); }
    return {};
}
void keep(Registry &registry) {
    registry.mir("keep", [](const Input &input, MIRBuilder &out, const Ctx &, const Registry &) -> Result {
        out.emit("mir.keep", input.operands);
        return {};
    });
}
void value(Registry &registry) {
    registry.mir("value", [](const Input &input, MIRBuilder &out, const Ctx &, const Registry &) -> Result {
        return out.emitValue("mir.value", input.operands, input.source.type);
    });
}
}

TEST(HIRToMIR, DefiningBlockStoredAfterConsumingBlock) {
    Registry registry; value(registry); keep(registry);
    HIRBuilder hir;
    auto consumer = hir.createBlock("consumer");
    auto producer = hir.createBlock("producer");
    hir.terminate("jump", {}, {producer});
    hir.setCurrentBlock(producer);
    auto produced = hir.emitValue("value", {}, ValueType::Int);
    hir.terminate("jump", {}, {consumer});
    hir.setCurrentBlock(consumer);
    hir.emit("keep", {Operand::fromValue(produced)});
    hir.terminate("return");
    auto result = HIRLoweringPass(registry).lower(hir.finish());
    CHECK(result.blocks.size() == 3);
    CHECK(result.blocks[1].instructions[0].operands[0].asValue().value ==
          result.blocks[2].instructions[0].result->value);
}

TEST(HIRToMIR, CyclicCFGVisitsDominatorsBeforeConsumers) {
    Registry registry; value(registry); keep(registry);
    HIRBuilder hir;
    auto consumer = hir.createBlock("consumer");
    auto producer = hir.createBlock("producer");
    auto exit = hir.createBlock("exit");
    hir.terminate("jump", {}, {producer});
    hir.setCurrentBlock(producer);
    auto v = hir.emitValue("value", {}, ValueType::Int);
    hir.terminate("jump", {}, {consumer});
    hir.setCurrentBlock(consumer);
    hir.emit("keep", {Operand::fromValue(v)});
    hir.terminate("branch", {}, {producer, exit});
    hir.setCurrentBlock(exit);
    hir.terminate("return");
    auto mir = HIRLoweringPass(registry).lower(hir.finish(), {{true}});
    CHECK(mir.blocks[1].instructions[0].operands[0].asValue().value ==
          mir.blocks[2].instructions[0].result->value);
}

TEST(HIRToMIR, RemapsOperandsAutomaticallyWhenProducerEmitsMultipleValues) {
    Registry registry;
    registry.mir("two", [](const Input &, MIRBuilder &out, const Ctx &, const Registry &) -> Result {
        out.emitValue("temporary", {}, ValueType::Int);
        return out.emitValue("actual", {}, ValueType::Int);
    });
    keep(registry);
    HIRBuilder hir;
    auto v = hir.emitValue("two", {}, ValueType::Int);
    hir.emit("keep", {Operand::fromValue(v)});
    hir.terminate("return");
    auto mir = HIRLoweringPass(registry).lower(hir.finish());
    CHECK(mir.blocks[0].instructions.size() == 3);
    CHECK(mir.blocks[0].instructions[2].operands[0].asValue().value == 1);
    CHECK(mir.blocks[0].instructions[1].result->value == 1);
}

TEST(HIRToMIR, DetectsSilentLossOfValueResult) {
    Registry registry;
    registry.mir("drop", [](const Input &, MIRBuilder &, const Ctx &, const Registry &) -> Result { return {}; });
    HIRBuilder hir;
    hir.emitValue("drop", {}, ValueType::Int);
    hir.terminate("return");
    auto message = failure([&] { HIRLoweringPass(registry).lower(hir.finish()); });
    CHECK(message.find("block 0") != std::string::npos);
    CHECK(message.find("instruction 0") != std::string::npos);
    CHECK(message.find("silently discarded") != std::string::npos);
}

TEST(HIRToMIR, RejectsUndeclaredMissingAndVoidResultMappings) {
    Registry bad;
    bad.mir("ghost", [](const Input &, MIRBuilder &, const Ctx &, const Registry &) -> Result {
        return ValueId{900};
    });
    HIRBuilder hir;
    hir.emitValue("ghost", {}, ValueType::Int);
    hir.terminate("return");
    CHECK(failure([&] { HIRLoweringPass(bad).lower(hir.finish()); }).find("undefined MIR value") != std::string::npos);
    Registry voidReg;
    voidReg.mir("ghost", [](const Input &, MIRBuilder &out, const Ctx &, const Registry &) -> Result {
        return out.emitValue("temporary", {}, ValueType::Int);
    });
    HIRBuilder plain;
    plain.emit("ghost"); plain.terminate("return");
    CHECK(failure([&] { HIRLoweringPass(voidReg).lower(plain.finish()); }).find("void HIR") != std::string::npos);
}

TEST(HIRToMIR, RequiresExplicitConversionForTypeChanges) {
    Registry bad;
    bad.mir("change", [](const Input &, MIRBuilder &out, const Ctx &, const Registry &) -> Result {
        return out.emitValue("changed", {}, ValueType::Bool);
    });
    HIRBuilder builder;
    builder.emitValue("change", {}, ValueType::Int);
    builder.terminate("return");
    auto hir = builder.finish();
    CHECK(failure([&] { HIRLoweringPass(bad).lower(hir); }).find("without declared conversion") != std::string::npos);
    Registry good;
    good.mir("change", [](const Input &, MIRBuilder &out, const Ctx &, const Registry &) -> Result {
        return Result::converted(out.emitValue("changed", {}, ValueType::Bool));
    });
    CHECK(HIRLoweringPass(good).lower(hir).blocks[0].instructions[0].type == ValueType::Bool);
}

TEST(HIRToMIR, ElisionMustBeExplicitAndCannotLeaveLiveUses) {
    Registry registry;
    registry.mir("gone", [](const Input &, MIRBuilder &, const Ctx &, const Registry &) -> Result {
        return Result::elided();
    });
    keep(registry);
    HIRBuilder first;
    first.emitValue("gone", {}, ValueType::Int); first.terminate("return");
    CHECK(HIRLoweringPass(registry).lower(first.finish()).blocks[0].instructions.empty());
    HIRBuilder second;
    auto v = second.emitValue("gone", {}, ValueType::Int);
    second.emit("keep", {Operand::fromValue(v)}); second.terminate("return");
    CHECK(failure([&] { HIRLoweringPass(registry).lower(second.finish()); }).find("unknown HIR value") != std::string::npos);
}

TEST(HIRToMIR, RejectsAliasThatDoesNotDominateEvenWhenUnused) {
    Registry registry;
    registry.mir("capture", [](const Input &, MIRBuilder &out, const Ctx &, const Registry &) -> Result {
        return out.emitValue("one", {}, ValueType::Int);
    });
    registry.mir("alias", [](const Input &, MIRBuilder &, const Ctx &, const Registry &) -> Result {
        return ValueId{0}; // exists, but only in the sibling branch
    });
    HIRBuilder hir;
    auto left = hir.createBlock("left"), right = hir.createBlock("right");
    hir.terminate("branch", {}, {left, right});
    hir.setCurrentBlock(left);
    hir.emitValue("capture", {}, ValueType::Int);
    hir.terminate("return");
    hir.setCurrentBlock(right);
    hir.emitValue("alias", {}, ValueType::Int);
    hir.terminate("return");
    CHECK(failure([&] { HIRLoweringPass(registry).lower(hir.finish()); }).find("invalid mapped result") != std::string::npos);
}

TEST(HIRToMIR, DetectsChangeOfInsertionBlockEvenWhenRestored) {
    Registry registry;
    registry.mir("move", [](const Input &, MIRBuilder &out, const Ctx &, const Registry &) -> Result {
        out.setCurrentBlock(BlockId{1});
        out.setCurrentBlock(BlockId{0});
        return {};
    });
    HIRBuilder hir;
    auto second = hir.createBlock("second");
    hir.emit("move"); hir.terminate("jump", {}, {second});
    hir.setCurrentBlock(second); hir.terminate("return");
    CHECK(failure([&] { HIRLoweringPass(registry).lower(hir.finish()); }).find("insertion block") != std::string::npos);
}

TEST(HIRToMIR, PreservesNamesAndOpaqueMetadataOnMultipleEmissions) {
    Registry registry;
    registry.mir("two", [](const Input &, MIRBuilder &out, const Ctx &, const Registry &) -> Result {
        out.emit("first"); out.emit("second"); return {};
    });
    HIRBuilder hir;
    hir.emit("two");
    hir.terminate("return");
    auto source = hir.finish();
    source.blocks[0].name = "custom entry";
    source.metadata.set<int>("module", 10);
    source.blocks[0].metadata.set<int>("block", 20);
    source.blocks[0].instructions[0].metadata.set<int>("source", 30);
    source.blocks[0].instructions[0].metadata.freeze();
    source.blocks[0].terminator->metadata.set<int>("terminator", 40);
    auto mir = HIRLoweringPass(registry).lower(source);
    CHECK(mir.blocks[0].name == "custom entry");
    CHECK(mir.metadata.require<int>("module") == 10);
    CHECK(mir.blocks[0].metadata.require<int>("block") == 20);
    CHECK(mir.blocks[0].instructions.size() == 2);
    for (const auto &inst : mir.blocks[0].instructions) {
        CHECK(inst.metadata.require<int>("source") == 30);
        CHECK(inst.metadata.frozen());
    }
    CHECK(mir.blocks[0].terminator->metadata.require<int>("terminator") == 40);
    auto noMeta = HIRLoweringPass(registry).lower(source, {{true}, false});
    CHECK(noMeta.metadata.empty());
    CHECK(noMeta.blocks[0].instructions[0].metadata.empty());
}

TEST(HIRToMIR, CompleteRequiresTerminatorsPartialAllowsThemMissing) {
    Registry registry; keep(registry);
    HIRBuilder hir;
    hir.emit("keep");
    auto source = hir.finish();
    CHECK(HIRLoweringPass(registry).lower(source).blocks[0].terminator == std::nullopt);
    CHECK(failure([&] { HIRLoweringPass(registry).lower(source, {{true}}); }).find("missing terminator") != std::string::npos);
}

TEST(HIRToMIR, DiagnosticsPreserveCallbackErrorAndOperation) {
    Registry registry;
    registry.mir("panic", [](const Input &, MIRBuilder &, const Ctx &, const Registry &) -> Result {
        throw std::runtime_error("the lowerer failed");
    });
    HIRBuilder hir;
    hir.emit("panic"); hir.terminate("return");
    auto source = hir.finish();
    auto message = failure([&] { HIRLoweringPass(registry).lower(source); });
    CHECK(message.find("'panic'") != std::string::npos);
    CHECK(message.find("the lowerer failed") != std::string::npos);
}

TEST(HIRToMIR, AllowsValidatedAliasAndRemapsFutureUses) {
    Registry registry; value(registry); keep(registry);
    registry.mir("alias", [](const Input &input, MIRBuilder &, const Ctx &, const Registry &) -> Result {
        return input.operands[0].asValue();
    });
    HIRBuilder hir;
    auto first = hir.emitValue("value", {}, ValueType::Int);
    auto alias = hir.emitValue("alias", {Operand::fromValue(first)}, ValueType::Int);
    hir.emit("keep", {Operand::fromValue(alias)});
    hir.terminate("return");
    auto mir = HIRLoweringPass(registry).lower(hir.finish());
    CHECK(mir.blocks[0].instructions.size() == 2);
    CHECK(mir.blocks[0].instructions[1].operands[0].asValue().value == 0);
}

TEST(HIRToMIR, DisallowsEmissionsIntoDifferentBlockDuringCallback) {
    Registry registry;
    registry.mir("move", [](const Input &, MIRBuilder &out, const Ctx &, const Registry &) -> Result {
        out.setCurrentBlock(BlockId{1});
        out.emit("unwanted");
        out.setCurrentBlock(BlockId{0});
        return {};
    });
    HIRBuilder builder;
    auto next = builder.createBlock("next");
    builder.emit("move"); builder.terminate("jump", {}, {next});
    builder.setCurrentBlock(next); builder.terminate("return");
    CHECK(failure([&] { HIRLoweringPass(registry).lower(builder.finish()); }).find("insertion block") != std::string::npos);
}

TEST(HIRToMIR, ReportsMissingLowererWithInstructionContext) {
    Registry registry;
    HIRBuilder builder;
    builder.emit("nonexistent"); builder.terminate("return");
    auto message = failure([&] { HIRLoweringPass(registry).lower(builder.finish()); });
    CHECK(message.find("block 0") != std::string::npos);
    CHECK(message.find("nonexistent") != std::string::npos);
}

TEST(HIRToMIR, LargeReachableCFGDoesNotNeedQuadraticDominanceTable) {
    Registry registry; value(registry); keep(registry);
    HIRBuilder builder;
    std::vector<BlockId> blocks{};
    for (int i = 1; i < 1300; ++i) blocks.push_back(builder.createBlock("step"));
    auto shared = builder.emitValue("value", {}, ValueType::Int);
    builder.terminate("jump", {}, {blocks[0]});
    for (std::size_t i = 0; i < blocks.size(); ++i) {
        builder.setCurrentBlock(blocks[i]);
        builder.emit("keep", {Operand::fromValue(shared)});
        if (i+1 == blocks.size()) builder.terminate("return");
        else builder.terminate("jump", {}, {blocks[i+1]});
    }
    auto mir = HIRLoweringPass(registry).lower(builder.finish(), {{true}});
    CHECK(mir.blocks.size() == 1300);
    CHECK(mir.blocks.back().instructions[0].operands[0].asValue().value == 0);
}
