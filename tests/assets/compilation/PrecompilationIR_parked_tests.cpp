#include "../../tester.hpp"
#include "novac/assets/compilation/ir/IR.hpp"
#include <stdexcept>
#include <utility>
#include <vector>
namespace {
using namespace novac;
template<class Fn> bool fails(Fn &&fn) { try { fn(); } catch (const std::runtime_error &) { return true; } return false; }
ir::Instruction valueInstruction(std::uint32_t id, std::vector<ir::Operand> operands = {}) {
    return {"op", ir::ValueId{id}, std::move(operands), ir::ValueType::Int};
}
}
TEST(PrecompilationIR, BuildersRejectInvalidBranchWithoutMutatingBlock) {
    ir::HIRBuilder hir{};
    ir::MIRBuilder mir{};
    CHECK(fails([&] { hir.terminate("jump", {}, {ir::BlockId{9999}}); }));
    CHECK(fails([&] { mir.terminate("jump", {}, {ir::BlockId{9999}}); }));
    hir.terminate("return");
    mir.terminate("return");
    CHECK(hir.finish().blocks[0].terminator->op == "return");
    CHECK(mir.finish().blocks[0].terminator->op == "return");
}

TEST(PrecompilationIR, RejectsBadBlockIdsAndTargetsInUserConstructedModules) {
    ir::HIRModule module{};
    module.blocks.push_back({ir::BlockId{2}, "wrong"});
    CHECK(fails([&] { ir::validate(module); }));
    module.blocks[0].id = ir::BlockId{0};
    module.blocks[0].terminator = ir::Terminator{"jump", {}, {ir::BlockId{99}}};
    CHECK(fails([&] { ir::validate(module); }));
    module.blocks[0].terminator = ir::Terminator{"return"};
    ir::validate(module, {true});
}

TEST(PrecompilationIR, RejectsUndefinedDuplicateAndVoidResults) {
    ir::MIRModule module{};
    module.blocks.push_back({ir::BlockId{0}, "entry"});
    auto &instructions = module.blocks[0].instructions;
    instructions.push_back(valueInstruction(0, {ir::Operand::fromValue(ir::ValueId{12})}));
    CHECK(fails([&] { ir::validate(module); }));
    instructions[0] = valueInstruction(0);
    instructions.push_back(valueInstruction(0));
    CHECK(fails([&] { ir::validate(module); }));
    instructions.pop_back();
    instructions[0].type = ir::ValueType::Void;
    CHECK(fails([&] { ir::validate(module); }));
    instructions[0].type = ir::ValueType::Int;
    ir::validate(module);
}

TEST(PrecompilationIR, EnforcesDefinitionBeforeUseAndDominance) {
    ir::HIRModule module{};
    module.blocks.push_back({ir::BlockId{0}, "entry"});
    module.blocks.push_back({ir::BlockId{1}, "left"});
    module.blocks.push_back({ir::BlockId{2}, "right"});
    module.blocks.push_back({ir::BlockId{3}, "join"});
    module.blocks[0].terminator = ir::Terminator{"branch", {}, {ir::BlockId{1},ir::BlockId{2}}};
    module.blocks[1].instructions.push_back(valueInstruction(1));
    module.blocks[1].terminator = ir::Terminator{"jump", {}, {ir::BlockId{3}}};
    module.blocks[2].terminator = ir::Terminator{"jump", {}, {ir::BlockId{3}}};
    module.blocks[3].instructions.push_back(valueInstruction(2,{ir::Operand::fromValue(ir::ValueId{1})}));
    module.blocks[3].terminator = ir::Terminator{"return"};
    CHECK(fails([&] { ir::validate(module, {true}); })); // left doesn't dominate join
    module.blocks[0].instructions.push_back(valueInstruction(1));
    module.blocks[1].instructions.clear();
    ir::validate(module, {true});
    module.blocks[0].instructions.insert(module.blocks[0].instructions.begin(),
        valueInstruction(4, {ir::Operand::fromValue(ir::ValueId{1})}));
    CHECK(fails([&] { ir::validate(module); })); // same-block use before definition
}

TEST(PrecompilationIR, ForwardBranchesLowerAfterAllBlocksAreRegistered) {
    ir::LoweringRegistry registry{};
    registry.mir("keep", [](const ir::Instruction &, ir::MIRBuilder &builder,
                              ir::MIRLoweringContext &, const ir::LoweringRegistry &) -> std::optional<ir::ValueId> {
        builder.emit("kept");
        return std::nullopt;
    });
    ir::HIRBuilder builder{};
    const auto dest = builder.createBlock("dest");
    builder.terminate("jump", {}, {dest});
    builder.setCurrentBlock(dest);
    builder.emit("keep");
    builder.terminate("return");
    const auto mir = ir::HIRLoweringPass{registry}.lower(builder.finish());
    CHECK(mir.blocks.size() == 2);
    CHECK(mir.blocks[0].terminator->successors[0].target.value == dest.value);
    CHECK(mir.blocks[1].instructions[0].op == "kept");
}
