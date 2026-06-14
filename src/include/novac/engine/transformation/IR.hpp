#pragma once

#include "../syntax/Node.hpp"
#include "../foundation/Ids.hpp"
#include "../foundation/registry/Registry.hpp"
#include "../foundation/registry/RegistryHelpers.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace novac::ir {

/**
 * @brief Identifier for an SSA-style value produced by an instruction.
 */
struct ValueId {
    std::uint32_t value{};
};

/**
 * @brief Identifier for a basic block in an IR module.
 */
struct BlockId {
    std::uint32_t value{};
};

/**
 * @brief Static value category attached to IR instruction results.
 */
enum class ValueType {
    /**
     * @brief Value type has not been inferred or specified.
     */
    Unknown,

    /**
     * @brief Instruction does not produce a usable value.
     */
    Void,

    /**
     * @brief Integer value.
     */
    Int,

    /**
     * @brief Boolean value.
     */
    Bool,

    /**
     * @brief Floating-point value.
     */
    Float,

    /**
     * @brief String value.
     */
    String
};

/**
 * @brief Literal constant stored directly in IR operands.
 */
class Literal {
public:
    using Data = std::variant<std::monostate, int, double, bool, std::string>;

    /**
     * @brief Creates an empty literal.
     */
    Literal();

    /**
     * @brief Creates a literal from variant storage.
     *
     * @param data Literal payload.
     */
    explicit Literal(Data data);

    /**
     * @brief Creates an integer literal.
     *
     * @param value Integer value.
     * @return Literal containing the value.
     */
    static Literal integer(int value);

    /**
     * @brief Creates a floating-point literal.
     *
     * @param value Floating-point value.
     * @return Literal containing the value.
     */
    static Literal floating(double value);

    /**
     * @brief Creates a boolean literal.
     *
     * @param value Boolean value.
     * @return Literal containing the value.
     */
    static Literal boolean(bool value);

    /**
     * @brief Creates a string literal.
     *
     * @param value String value.
     * @return Literal containing the value.
     */
    static Literal string(std::string value);

    /**
     * @brief Converts the literal to a display string.
     *
     * @return Text representation of the literal.
     */
    std::string toString() const;

private:
    Data data_;
};

/**
 * @brief Instruction operand referencing a value, literal, or symbol.
 */
class Operand {
public:
    using Data = std::variant<ValueId, Literal, std::string>;

    /**
     * @brief Creates an operand from variant storage.
     *
     * @param data Operand payload.
     */
    explicit Operand(Data data);

    /**
     * @brief Creates an operand referencing an IR value.
     *
     * @param id Value identifier.
     * @return Value operand.
     */
    static Operand fromValue(ValueId id);

    /**
     * @brief Creates an operand from a literal.
     *
     * @param literal Literal value.
     * @return Literal operand.
     */
    static Operand fromLiteral(Literal literal);

    /**
     * @brief Creates an integer literal operand.
     *
     * @param value Integer value.
     * @return Literal operand.
     */
    static Operand fromLiteral(int value);

    /**
     * @brief Creates a floating-point literal operand.
     *
     * @param value Floating-point value.
     * @return Literal operand.
     */
    static Operand fromLiteral(double value);

    /**
     * @brief Creates a boolean literal operand.
     *
     * @param value Boolean value.
     * @return Literal operand.
     */
    static Operand fromLiteral(bool value);

    /**
     * @brief Creates a string literal operand.
     *
     * @param value String value.
     * @return Literal operand.
     */
    static Operand fromLiteral(std::string value);

    /**
     * @brief Creates a symbolic operand.
     *
     * @param name Symbol name.
     * @return Symbol operand.
     */
    static Operand fromSymbol(std::string name);

    /**
     * @brief Checks whether this operand references an IR value.
     *
     * @return True if the operand stores a ValueId.
     */
    bool isValue() const;

    /**
     * @brief Returns this operand as an IR value reference.
     *
     * @return Stored value identifier.
     *
     * @throws std::runtime_error If the operand is not a value reference.
     */
    ValueId asValue() const;

    /**
     * @brief Returns the underlying operand storage.
     *
     * @return Operand payload.
     */
    const Data &data() const;

    /**
     * @brief Converts the operand to a display string.
     *
     * @return Text representation of the operand.
     */
    std::string toString() const;

private:
    Data data_;
};

/**
 * @brief Non-terminating IR instruction.
 */
struct Instruction {
    std::string op{};
    std::optional<ValueId> result{};
    std::vector<Operand> operands{};
    ValueType type{ValueType::Unknown};
};

/**
 * @brief Basic block terminator instruction.
 */
struct Terminator {
    std::string op{};
    std::vector<Operand> operands{};
    std::vector<BlockId> targets{};
};

/**
 * @brief Sequence of IR instructions with an optional terminator.
 */
struct BasicBlock {
    BlockId id{};
    std::string name{};
    std::vector<Instruction> instructions{};
    std::optional<Terminator> terminator{};
};

/**
 * @brief High-level intermediate representation module.
 */
struct HIRModule {
    std::vector<BasicBlock> blocks{};
};

/**
 * @brief Medium-level intermediate representation module.
 */
struct MIRModule {
    std::vector<BasicBlock> blocks{};
};

/**
 * @brief Builds HIR modules incrementally.
 *
 * The builder owns the module under construction and returns it by value
 * when finish() is called.
 */
class HIRBuilder {
public:
    /**
     * @brief Creates a HIR builder with an entry block.
     */
    HIRBuilder();

    /**
     * @brief Creates a new basic block.
     *
     * @param name Block name.
     * @return Identifier of the created block.
     */
    BlockId createBlock(std::string name);

    /**
     * @brief Changes the block receiving emitted instructions.
     *
     * @param block Target block identifier.
     *
     * @throws std::runtime_error If the block identifier is invalid.
     */
    void setCurrentBlock(BlockId block);

    /**
     * @brief Emits an instruction that produces a value.
     *
     * @param op Instruction operation name.
     * @param operands Instruction operands.
     * @param type Result value type.
     * @return Identifier of the produced value.
     *
     * @throws std::runtime_error If the current block already has a terminator.
     */
    ValueId emitValue(std::string op, std::vector<Operand> operands = {}, ValueType type = ValueType::Unknown);

    /**
     * @brief Emits an instruction that produces a value.
     *
     * @param op Instruction operation identifier.
     * @param operands Instruction operands.
     * @param type Result value type.
     * @return Identifier of the produced value.
     *
     * @throws std::runtime_error If the current block already has a terminator.
     */
    ValueId emitValue(const ids::Operation &op, std::vector<Operand> operands = {}, ValueType type = ValueType::Unknown);

    /**
     * @brief Emits an instruction with no result value.
     *
     * @param op Instruction operation name.
     * @param operands Instruction operands.
     *
     * @throws std::runtime_error If the current block already has a terminator.
     */
    void emit(std::string op, std::vector<Operand> operands = {});

    /**
     * @brief Emits an instruction with no result value.
     *
     * @param op Instruction operation identifier.
     * @param operands Instruction operands.
     *
     * @throws std::runtime_error If the current block already has a terminator.
     */
    void emit(const ids::Operation &op, std::vector<Operand> operands = {});

    /**
     * @brief Terminates the current block.
     *
     * @param op Terminator operation name.
     * @param operands Terminator operands.
     * @param targets Target block identifiers.
     *
     * @throws std::runtime_error If op is empty or the current block already has a terminator.
     */
    void terminate(std::string op, std::vector<Operand> operands = {}, std::vector<BlockId> targets = {});

    /**
     * @brief Terminates the current block.
     *
     * @param op Terminator operation identifier.
     * @param operands Terminator operands.
     * @param targets Target block identifiers.
     *
     * @throws std::runtime_error If the current block already has a terminator.
     */
    void terminate(const ids::Operation &op, std::vector<Operand> operands = {}, std::vector<BlockId> targets = {});

    /**
     * @brief Completes the module and transfers it out of the builder.
     *
     * @return Built HIR module.
     */
    HIRModule finish();

private:
    BasicBlock &currentBlock();
    const BasicBlock &currentBlock() const;

    HIRModule module_;
    BlockId currentBlock_;
    std::uint32_t nextValue_;
};

/**
 * @brief Builds MIR modules incrementally.
 *
 * The builder owns the module under construction and returns it by value
 * when finish() is called.
 */
class MIRBuilder {
public:
    /**
     * @brief Creates a MIR builder with an entry block.
     */
    MIRBuilder();

    /**
     * @brief Creates a new basic block.
     *
     * @param name Block name.
     * @return Identifier of the created block.
     */
    BlockId createBlock(std::string name);

    /**
     * @brief Changes the block receiving emitted instructions.
     *
     * @param block Target block identifier.
     *
     * @throws std::runtime_error If the block identifier is invalid.
     */
    void setCurrentBlock(BlockId block);

    /**
     * @brief Emits an instruction that produces a value.
     *
     * @param op Instruction operation name.
     * @param operands Instruction operands.
     * @param type Result value type.
     * @return Identifier of the produced value.
     *
     * @throws std::runtime_error If the current block already has a terminator.
     */
    ValueId emitValue(std::string op, std::vector<Operand> operands = {}, ValueType type = ValueType::Unknown);

    /**
     * @brief Emits an instruction that produces a value.
     *
     * @param op Instruction operation identifier.
     * @param operands Instruction operands.
     * @param type Result value type.
     * @return Identifier of the produced value.
     *
     * @throws std::runtime_error If the current block already has a terminator.
     */
    ValueId emitValue(const ids::Operation &op, std::vector<Operand> operands = {}, ValueType type = ValueType::Unknown);

    /**
     * @brief Emits an instruction with no result value.
     *
     * @param op Instruction operation name.
     * @param operands Instruction operands.
     *
     * @throws std::runtime_error If the current block already has a terminator.
     */
    void emit(std::string op, std::vector<Operand> operands = {});

    /**
     * @brief Emits an instruction with no result value.
     *
     * @param op Instruction operation identifier.
     * @param operands Instruction operands.
     *
     * @throws std::runtime_error If the current block already has a terminator.
     */
    void emit(const ids::Operation &op, std::vector<Operand> operands = {});

    /**
     * @brief Terminates the current block.
     *
     * @param op Terminator operation name.
     * @param operands Terminator operands.
     * @param targets Target block identifiers.
     *
     * @throws std::runtime_error If op is empty or the current block already has a terminator.
     */
    void terminate(std::string op, std::vector<Operand> operands = {}, std::vector<BlockId> targets = {});

    /**
     * @brief Terminates the current block.
     *
     * @param op Terminator operation identifier.
     * @param operands Terminator operands.
     * @param targets Target block identifiers.
     *
     * @throws std::runtime_error If the current block already has a terminator.
     */
    void terminate(const ids::Operation &op, std::vector<Operand> operands = {}, std::vector<BlockId> targets = {});

    /**
     * @brief Completes the module and transfers it out of the builder.
     *
     * @return Built MIR module.
     */
    MIRModule finish();

private:
    BasicBlock &currentBlock();
    const BasicBlock &currentBlock() const;

    MIRModule module_;
    BlockId currentBlock_;
    std::uint32_t nextValue_;
};

/**
 * @brief Tracks value mappings while lowering HIR to MIR.
 */
class MIRLoweringContext {
public:
    /**
     * @brief Associates a HIR value with a MIR value.
     *
     * @param hirValue Source HIR value.
     * @param mirValue Replacement MIR value.
     */
    void bind(ValueId hirValue, ValueId mirValue);

    /**
     * @brief Checks whether a HIR value has a MIR mapping.
     *
     * @param hirValue HIR value to test.
     * @return True if the value has been mapped.
     */
    bool has(ValueId hirValue) const;

    /**
     * @brief Resolves a HIR value to its mapped MIR value.
     *
     * @param hirValue HIR value to resolve.
     * @return Mapped MIR value.
     *
     * @throws std::runtime_error If the HIR value has no mapping.
     */
    ValueId resolve(ValueId hirValue) const;

    /**
     * @brief Remaps a value operand through the current HIR-to-MIR bindings.
     *
     * Non-value operands are returned unchanged.
     *
     * @param operand Operand to remap.
     * @return Remapped operand.
     *
     * @throws std::runtime_error If operand references an unmapped HIR value.
     */
    Operand remapOperand(const Operand &operand) const;

    /**
     * @brief Remaps each operand through the current HIR-to-MIR bindings.
     *
     * @param operands Operands to remap.
     * @return Remapped operand list.
     *
     * @throws std::runtime_error If an operand references an unmapped HIR value.
     */
    std::vector<Operand> remapOperands(const std::vector<Operand> &operands) const;

private:
    std::unordered_map<std::uint32_t, ValueId> values_;
};

/**
 * @brief Registry of lowering functions for AST-to-HIR and HIR-to-MIR passes.
 */
class LoweringRegistry {
public:
    using LoweredValue = std::optional<ValueId>;
    using HIRLowerer = std::function<LoweredValue(const ast::Node &, HIRBuilder &, const LoweringRegistry &)>;
    using MIRLowerer = std::function<LoweredValue(const Instruction &, MIRBuilder &, MIRLoweringContext &, const LoweringRegistry &)>;

    /**
     * @brief Creates a lowering registry.
     *
     * @param duplicatePolicy Policy used when duplicate lowerers are registered.
     */
    explicit LoweringRegistry(registry::DuplicatePolicy duplicatePolicy = registry::DuplicatePolicy::Error);

    /**
     * @brief Registers an AST-to-HIR lowerer for a node kind.
     *
     * @param nodeKind AST node kind handled by the lowerer.
     * @param fn Lowering function.
     * @return Registration result.
     *
     * @throws std::runtime_error If nodeKind is empty, fn is empty, or duplicate registration is rejected.
     */
    registry::RegisterStatus hir(std::string nodeKind, HIRLowerer fn);

    /**
     * @brief Registers an AST-to-HIR lowerer for a node kind.
     *
     * @param nodeKind AST node kind handled by the lowerer.
     * @param fn Lowering function.
     * @return Registration result.
     *
     * @throws std::runtime_error If fn is empty or duplicate registration is rejected.
     */
    registry::RegisterStatus hir(const ids::NodeKind &nodeKind, HIRLowerer fn);

    /**
     * @brief Registers a HIR-to-MIR lowerer for an instruction operation.
     *
     * @param hirKind HIR instruction operation handled by the lowerer.
     * @param fn Lowering function.
     * @return Registration result.
     *
     * @throws std::runtime_error If hirKind is empty, fn is empty, or duplicate registration is rejected.
     */
    registry::RegisterStatus mir(std::string hirKind, MIRLowerer fn);

    /**
     * @brief Registers a HIR-to-MIR lowerer for an instruction operation.
     *
     * @param hirKind HIR instruction operation handled by the lowerer.
     * @param fn Lowering function.
     * @return Registration result.
     *
     * @throws std::runtime_error If fn is empty or duplicate registration is rejected.
     */
    registry::RegisterStatus mir(const ids::Operation &hirKind, MIRLowerer fn);

    /**
     * @brief Checks whether an AST-to-HIR lowerer exists.
     *
     * @param nodeKind AST node kind.
     * @return True if a lowerer is registered.
     */
    bool hasHIR(const std::string &nodeKind) const;

    /**
     * @brief Checks whether an AST-to-HIR lowerer exists.
     *
     * @param nodeKind AST node kind.
     * @return True if a lowerer is registered.
     */
    bool hasHIR(const ids::NodeKind &nodeKind) const;

    /**
     * @brief Checks whether a HIR-to-MIR lowerer exists.
     *
     * @param hirKind HIR instruction operation.
     * @return True if a lowerer is registered.
     */
    bool hasMIR(const std::string &hirKind) const;

    /**
     * @brief Checks whether a HIR-to-MIR lowerer exists.
     *
     * @param hirKind HIR instruction operation.
     * @return True if a lowerer is registered.
     */
    bool hasMIR(const ids::Operation &hirKind) const;

    /**
     * @brief Lowers an AST node into HIR.
     *
     * @param node AST node to lower.
     * @param out HIR builder receiving emitted instructions.
     * @return Optional produced value.
     *
     * @throws std::runtime_error If no HIR lowerer is registered for the node kind.
     */
    LoweredValue lowerHIR(const ast::Node &node, HIRBuilder &out) const;

    /**
     * @brief Lowers a HIR instruction into MIR.
     *
     * If both the source instruction and lowered result have values, the context
     * is updated with the HIR-to-MIR value mapping.
     *
     * @param instruction HIR instruction to lower.
     * @param out MIR builder receiving emitted instructions.
     * @param context HIR-to-MIR lowering context.
     * @return Optional produced MIR value.
     *
     * @throws std::runtime_error If no MIR lowerer is registered for the instruction operation.
     */
    LoweredValue lowerMIR(const Instruction &instruction, MIRBuilder &out, MIRLoweringContext &context) const;

    /**
     * @brief Lowers all child nodes directly stored in a node.
     *
     * @param node AST node whose child fields are lowered.
     * @param out HIR builder receiving emitted instructions.
     */
    void lowerChildren(const ast::Node &node, HIRBuilder &out) const;

    /**
     * @brief Lowers child nodes in schema field order when a schema is available.
     *
     * Falls back to direct field traversal if the node kind has no schema.
     *
     * @param node AST node whose child fields are lowered.
     * @param nodes Node registry used to determine schema field order.
     * @param out HIR builder receiving emitted instructions.
     */
    void lowerChildren(const ast::Node &node, const ast::NodeRegistry &nodes, HIRBuilder &out) const;

private:
    std::unordered_map<std::string, HIRLowerer> hir_;
    std::unordered_map<std::string, MIRLowerer> mir_;
    registry::DuplicatePolicy duplicatePolicy_;
};

/**
 * @brief Lowers an AST into a HIR module.
 *
 * The pass references an external LoweringRegistry, which must remain valid
 * for the lifetime of the pass.
 */
class ASTLoweringPass {
public:
    /**
     * @brief Creates an AST lowering pass.
     *
     * @param registry Lowering registry used by the pass.
     */
    explicit ASTLoweringPass(const LoweringRegistry &registry);

    /**
     * @brief Lowers an AST root node into HIR.
     *
     * @param root AST root node.
     * @return Produced HIR module.
     */
    HIRModule lower(const ast::Node &root) const;

private:
    const LoweringRegistry &registry_;
};

/**
 * @brief Lowers a HIR module into a MIR module.
 *
 * The pass references an external LoweringRegistry, which must remain valid
 * for the lifetime of the pass.
 */
class HIRLoweringPass {
public:
    /**
     * @brief Creates a HIR lowering pass.
     *
     * @param registry Lowering registry used by the pass.
     */
    explicit HIRLoweringPass(const LoweringRegistry &registry);

    /**
     * @brief Lowers a HIR module into MIR.
     *
     * @param hir HIR module to lower.
     * @return Produced MIR module.
     */
    MIRModule lower(const HIRModule &hir) const;

private:
    const LoweringRegistry &registry_;
};

/**
 * @brief Converts a value type to its stable textual name.
 *
 * @param type Value type to convert.
 * @return Textual type name.
 */
std::string valueTypeName(ValueType type);

} // namespace novac::ir