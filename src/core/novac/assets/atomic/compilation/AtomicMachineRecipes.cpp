#include "novac/assets/atomic/compilation/AtomicMachineRecipes.hpp"

namespace novac::assets::atomic::compilation {
using namespace novac::compilation::machine;
void installIncrementI32(MachineRecipeRegistry &out, const novac::ir::TypeRef &i32) {
    MachineRecipe rule{};
    rule.id = "atomic.i32.increment.x64";
    rule.operation = "atomic.i32.increment";
    rule.resultType = i32;
    rule.operandTypes = std::vector<novac::ir::TypeRef>{i32};
    rule.tier = RecipeTier::Override;
    rule.emit = [](const novac::ir::Instruction &, MachineBuilder &b) {
        b.loadInput(0);
        b.addImmediate(1);
    };
    out.add(std::move(rule));
}
void installZeroConstantOverride(MachineRecipeRegistry &out, const novac::ir::TypeRef &i32) {
    MachineRecipe rule{};
    rule.id = "atomic.i32.zero.override.x64";
    rule.operation = "novac.i32.const";
    rule.resultType = i32;
    rule.tier = RecipeTier::Override;
    rule.matches = [](const novac::ir::Instruction &inst) {
        if (inst.operands.size()!=1) return false;
        const auto *literal = std::get_if<novac::ir::Literal>(&inst.operands[0].data());
        const int *number = literal ? std::get_if<int>(&literal->data()) : nullptr;
        return number && *number == 0;
    };
    // Deliberately different machine sequence, equivalent modulo 2^32:
    // mov eax,1; add eax,0xffffffff. Used to prove override really executes.
    rule.emit = [](const novac::ir::Instruction &, MachineBuilder &b) {
        b.loadImmediate(1); b.addImmediate(0xffffffffU);
    };
    out.add(std::move(rule));
}
} // namespace novac::assets::atomic::compilation
