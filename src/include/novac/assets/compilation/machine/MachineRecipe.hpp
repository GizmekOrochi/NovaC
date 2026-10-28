#pragma once

#include "novac/engine/transformation/IR.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace novac::compilation::machine {

/** Actual machine operations: operands are SSA references or immediates, not
 * unverified native bytes. The x86-64 encoder lowers these after checking
 * types and stack-slot ownership. MachineIR belongs to Compilation, NOT Engine. */
enum class MachineOpcode : std::uint8_t {
    LoadImmediate, LoadSSA, ConvertSSA, AddSSA, AddImmediate, SubSSA, MulSSA, LessThanSSA, UnsignedLessThanSSA, Nop, Trap
};
struct MachineInstruction {
    MachineOpcode opcode{};
    std::uint64_t immediate{};
    std::optional<ir::ValueId> source{};
};
struct MachineIR {
    std::vector<MachineInstruction> instructions{};
};

/** Small, verified stack-accumulator MachineIR builder (x86-64 M0 subset).
 * A recipe cannot modify the CFG, encode raw bytes or control the ABI. */
class MachineBuilder final {
public:
    explicit MachineBuilder(const ir::Instruction &source) : source_{source} {}
    void loadImmediate(std::uint32_t value);
    void loadImmediate64(std::uint64_t value);
    void convertInput(std::size_t index);
    void loadIntegerLiteral();
    void loadBooleanLiteral();
    void loadInput(std::size_t index);
    void addInput(std::size_t index);
    void addImmediate(std::uint32_t value);
    void subInput(std::size_t index);
    void mulInput(std::size_t index);
    void signedLessThanInput(std::size_t index);
    void unsignedLessThanInput(std::size_t index);
    /** Effect-only machine operations; do not fabricate SSA results. */
    void noOp();
    void trap();
    MachineIR finish() const;
private:
    const ir::Instruction &source_;
    MachineIR instructions_{};
    bool accumulatorReady_{false};
    ir::ValueId requireInput(std::size_t index) const;
    void append(MachineOpcode op, std::uint64_t immediate = 0,
                std::optional<ir::ValueId> source = std::nullopt);
};

using RecipeEmitter = std::function<void(const ir::Instruction &, MachineBuilder &)>;
enum class RecipeTier : std::uint8_t { Standard = 0, Inherited = 1, Override = 2 };
enum class OptimizationGoal : std::uint8_t { Debug, Speed, Size };
struct MachineTarget {
    std::string architecture{"x86-64"};
    std::set<std::string> cpuFeatures{};
    OptimizationGoal goal{OptimizationGoal::Speed};
};
struct RecipeCost {
    unsigned latency{1};
    unsigned bytes{1};
};
/** Operand shape checked before ranking recipes, never after selecting one. */
enum class OperandKind : std::uint8_t { Any, SSAValue, IntegerLiteral, BooleanLiteral };
struct OperandPattern {
    OperandKind kind{OperandKind::Any};
    std::optional<ir::TypeRef> type{}; // Only legal for SSAValue.
};
using SSATypeResolver = std::function<std::optional<ir::TypeRef>(ir::ValueId)>;

struct MachineRecipe {
    std::string id;
    std::string owner{"novac"}; // origin asset or feature, used in diagnostics
    std::string operation;
    std::string target{"x86-64"};
    // Missing type means generic matching. Exact TypeRef always wins over a
    // generic recipe of the same tier; no matching by spelling alone.
    std::optional<ir::TypeRef> resultType{};
    // When specified, verify every SSA input against these exact identities.
    // Empty list declares a literal-only or no-input recipe.
    std::optional<std::vector<ir::TypeRef>> operandTypes{};
    /** If present, describes the entire operand list including literals.
     * Unlike operandTypes, this can match non-SSA operands. */
    std::optional<std::vector<OperandPattern>> operandPatterns{};
    RecipeTier tier{RecipeTier::Standard};
    std::set<std::string> requiredCpuFeatures{};
    RecipeCost cost{};
    // Optional per-instruction guard (constant value, operand shape, etc.).
    std::function<bool(const ir::Instruction &)> matches{};
    RecipeEmitter emit{};
};

/** Deterministic, immutable-after-construction selection. Registration is
 * performed on a separate builder registry, then passed BY VALUE to Backend.
 * Conflict at equal specificity and cost is an error, not last-writer-wins. */
class MachineRecipeRegistry final {
public:
    void add(MachineRecipe recipe);
    void append(const MachineRecipeRegistry &other);
    void freeze() noexcept { frozen_ = true; }
    bool frozen() const noexcept { return frozen_; }
    /** Select only after checking operand shapes/types against the caller's SSA
     * definitions. Without a resolver, recipes requiring typed SSA operands
     * are ineligible rather than accidentally selected. */
    const MachineRecipe &select(const ir::Instruction &instruction,
                                const MachineTarget &target,
                                const SSATypeResolver &resolveType = {}) const;
    const std::vector<MachineRecipe> &recipes() const noexcept { return recipes_; }
private:
    std::vector<MachineRecipe> recipes_{};
    bool frozen_{false};
};

} // namespace novac::compilation::machine
