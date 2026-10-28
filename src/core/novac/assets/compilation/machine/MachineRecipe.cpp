#include "novac/assets/compilation/machine/MachineRecipe.hpp"

#include <algorithm>
#include <limits>
#include <tuple>
#include <utility>

namespace novac::compilation::machine {
void MachineBuilder::append(MachineOpcode op, std::uint64_t imm, std::optional<ir::ValueId> src) {
    if (op != MachineOpcode::LoadImmediate && op != MachineOpcode::LoadSSA && op != MachineOpcode::ConvertSSA && op != MachineOpcode::Nop && op != MachineOpcode::Trap && !accumulatorReady_)
        throw std::runtime_error("MachineIR: first instruction must define the accumulator");
    if (op == MachineOpcode::LoadImmediate || op == MachineOpcode::LoadSSA || op == MachineOpcode::ConvertSSA) accumulatorReady_ = true;
    instructions_.instructions.push_back({op,imm,src});
}
ir::ValueId MachineBuilder::requireInput(std::size_t index) const {
    if (index >= source_.operands.size() || !source_.operands[index].isValue())
        throw std::runtime_error("MachineIR: expected SSA input at index " + std::to_string(index));
    return source_.operands[index].asValue();
}
void MachineBuilder::loadImmediate(std::uint32_t imm) { append(MachineOpcode::LoadImmediate,imm); }
void MachineBuilder::loadImmediate64(std::uint64_t imm) { append(MachineOpcode::LoadImmediate,imm); }
void MachineBuilder::convertInput(std::size_t i) { append(MachineOpcode::ConvertSSA,0,requireInput(i)); }
void MachineBuilder::loadIntegerLiteral() {
    if (source_.operands.size() != 1) throw std::runtime_error("MachineIR: integer literal requires one operand");
    const auto *literal=std::get_if<ir::Literal>(&source_.operands[0].data());
    const auto *integer=literal ? std::get_if<int>(&literal->data()) : nullptr;
    if (!integer) throw std::runtime_error("MachineIR: i32 constant must be int");
    loadImmediate(static_cast<std::uint32_t>(*integer));
}
void MachineBuilder::loadBooleanLiteral() {
    if (source_.operands.size() != 1) throw std::runtime_error("MachineIR: boolean literal requires one operand");
    const auto *literal=std::get_if<ir::Literal>(&source_.operands[0].data());
    const auto *boolean=literal ? std::get_if<bool>(&literal->data()) : nullptr;
    if (!boolean) throw std::runtime_error("MachineIR: expected bool literal");
    loadImmediate(*boolean ? 1U : 0U);
}
void MachineBuilder::loadInput(std::size_t i) { append(MachineOpcode::LoadSSA,0,requireInput(i)); }
void MachineBuilder::addInput(std::size_t i) { append(MachineOpcode::AddSSA,0,requireInput(i)); }
void MachineBuilder::addImmediate(std::uint32_t imm) { append(MachineOpcode::AddImmediate,imm); }
void MachineBuilder::subInput(std::size_t i) { append(MachineOpcode::SubSSA,0,requireInput(i)); }
void MachineBuilder::mulInput(std::size_t i) { append(MachineOpcode::MulSSA,0,requireInput(i)); }
void MachineBuilder::signedLessThanInput(std::size_t i) { append(MachineOpcode::LessThanSSA,0,requireInput(i)); }
void MachineBuilder::unsignedLessThanInput(std::size_t i) { append(MachineOpcode::UnsignedLessThanSSA,0,requireInput(i)); }
void MachineBuilder::noOp() { append(MachineOpcode::Nop); }
void MachineBuilder::trap() { append(MachineOpcode::Trap); }
MachineIR MachineBuilder::finish() const {
    if (source_.result && !accumulatorReady_) throw std::runtime_error("MachineIR: recipe emitted no value");
    if (!source_.result && accumulatorReady_) throw std::runtime_error("MachineIR: void operation emitted a value");
    if (!source_.result && instructions_.instructions.empty())
        throw std::runtime_error("MachineIR: void recipe emitted nothing; use an explicit no-op");
    return instructions_;
}
void MachineRecipeRegistry::add(MachineRecipe recipe) {
    if (frozen_) throw std::runtime_error("MachineRecipeRegistry: registry is frozen");
    if (recipe.id.empty() || recipe.owner.empty() || recipe.operation.empty() || recipe.target.empty() || !recipe.emit)
        throw std::runtime_error("MachineRecipeRegistry: incomplete recipe");
    if (recipe.resultType && (!recipe.resultType->valid() || recipe.resultType->isVoid() || recipe.resultType->unresolved()))
        throw std::runtime_error("MachineRecipeRegistry: unresolved/invalid result type");
    if (recipe.operandTypes) {
        for (const auto &type : *recipe.operandTypes) {
            if (!type.valid() || type.isVoid() || type.unresolved())
                throw std::runtime_error("MachineRecipeRegistry: invalid SSA operand type");
        }
    }
    if (recipe.operandPatterns) {
        if (recipe.operandTypes && recipe.operandPatterns->size() != recipe.operandTypes->size())
            throw std::runtime_error("MachineRecipeRegistry: conflicting operand arities");
        for (const auto &operand : *recipe.operandPatterns) {
            if (operand.type && (operand.kind != OperandKind::SSAValue ||
                                 !operand.type->valid() || operand.type->isVoid() || operand.type->unresolved()))
                throw std::runtime_error("MachineRecipeRegistry: invalid typed operand pattern");
        }
    }
    if (recipe.cost.bytes == 0 || recipe.cost.latency == 0)
        throw std::runtime_error("MachineRecipeRegistry: cost estimates must be positive");
    if (std::any_of(recipes_.begin(),recipes_.end(),[&](const MachineRecipe& existing){return existing.id==recipe.id;}))
        throw std::runtime_error("MachineRecipeRegistry: duplicate recipe id '" + recipe.id + "'");
    recipes_.push_back(std::move(recipe));
}
void MachineRecipeRegistry::append(const MachineRecipeRegistry &other) {
    if (frozen_) throw std::runtime_error("MachineRecipeRegistry: registry is frozen");
    // Transactional merge: never leave half a pack installed after an error.
    MachineRecipeRegistry candidate{*this};
    for (const auto &recipe : other.recipes_) candidate.add(recipe);
    *this = std::move(candidate);
}
namespace {
bool matchesOperands(const MachineRecipe &recipe, const ir::Instruction &instruction,
                     const SSATypeResolver &resolveType) {
    const auto &operands = instruction.operands;
    if (recipe.operandTypes) {
        if (operands.size() != recipe.operandTypes->size() ||
            (!recipe.operandTypes->empty() && !resolveType)) return false;
        for (std::size_t i = 0; i < operands.size(); ++i) {
            if (!operands[i].isValue()) return false;
            const auto type = resolveType(operands[i].asValue());
            if (!type || *type != recipe.operandTypes->at(i)) return false;
        }
    }
    if (recipe.operandPatterns) {
        if (operands.size() != recipe.operandPatterns->size()) return false;
        for (std::size_t i = 0; i < operands.size(); ++i) {
            const auto &pattern = recipe.operandPatterns->at(i);
            const auto &operand = operands[i];
            switch (pattern.kind) {
                case OperandKind::Any: break;
                case OperandKind::SSAValue:
                    if (!operand.isValue()) return false;
                    break;
                case OperandKind::IntegerLiteral: {
                    const auto *lit = std::get_if<ir::Literal>(&operand.data());
                    if (!lit || !std::get_if<int>(&lit->data())) return false;
                    break;
                }
                case OperandKind::BooleanLiteral: {
                    const auto *lit = std::get_if<ir::Literal>(&operand.data());
                    if (!lit || !std::get_if<bool>(&lit->data())) return false;
                    break;
                }
            }
            if (pattern.type) {
                if (!resolveType || !operand.isValue()) return false;
                const auto type = resolveType(operand.asValue());
                if (!type || *type != *pattern.type) return false;
            }
        }
    }
    return true;
}
} // namespace

const MachineRecipe &MachineRecipeRegistry::select(const ir::Instruction &instruction,
                                                   const MachineTarget &target,
                                                   const SSATypeResolver &resolveType) const {
    const std::string &operation = instruction.op;
    const ir::TypeRef &type = instruction.type;
    const MachineRecipe *winner = nullptr;
    const MachineRecipe *tie = nullptr;
    using Rank = std::tuple<unsigned,unsigned,unsigned,unsigned,unsigned>;
    Rank best{};
    for (const auto &recipe : recipes_) {
        if (recipe.operation != operation || recipe.target != target.architecture ||
            (recipe.resultType && *recipe.resultType != type)) continue;
        if (!std::includes(target.cpuFeatures.begin(), target.cpuFeatures.end(),
                           recipe.requiredCpuFeatures.begin(), recipe.requiredCpuFeatures.end())) continue;
        if (!matchesOperands(recipe, instruction, resolveType)) continue;
        if (recipe.matches && !recipe.matches(instruction)) continue;
        // Rank all eligible recipes before deciding whether the best rank is
        // ambiguous. Lower-ranking ties must never mask a later override.
        const unsigned primaryCost = target.goal == OptimizationGoal::Size ? recipe.cost.bytes : recipe.cost.latency;
        const unsigned secondaryCost = target.goal == OptimizationGoal::Size ? recipe.cost.latency : recipe.cost.bytes;
        // Debug prefers unambiguous, generic stable recipes over optimizing for
        // latency; cost is ignored, so equal debug variants need explicit resolution.
        const Rank rank{
            static_cast<unsigned>(recipe.tier),
            recipe.resultType ? 1U : 0U,
            (recipe.operandTypes || recipe.operandPatterns) ? 1U : 0U,
            target.goal == OptimizationGoal::Debug ? 0U : std::numeric_limits<unsigned>::max() - primaryCost,
            target.goal == OptimizationGoal::Debug ? 0U : std::numeric_limits<unsigned>::max() - secondaryCost
        };
        if (!winner || rank > best) { winner = &recipe; tie = nullptr; best = rank; }
        else if (rank == best && !tie) tie = &recipe;
    }
    if (!winner) throw std::runtime_error("MachineRecipeRegistry: no compatible recipe for '" + operation +
                                          "' type '" + type.display() + "' target '" + target.architecture + "'");
    if (tie) throw std::runtime_error("MachineRecipeRegistry: ambiguous recipes '" + winner->id +
                                     "' and '" + tie->id + "' for '" + operation + "'");
    return *winner;
}
} // namespace novac::compilation::machine
