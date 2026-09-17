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
 * @brief Identifier for a value produced by an IR instruction.
 *
 * Value identifiers are used by other instructions to refer to a previous
 * result without storing the value directly in the instruction.
 */
struct ValueId {
    /** Numeric identifier unique inside the module being built. */
    std::uint32_t value{};
};

/**
 * @brief Identifier for a basic block in an IR module.
 *
 * Block identifiers are used by builders and terminators to reference
 * control-flow targets inside the same module.
 */
struct BlockId {
    /** Index of the block inside its IR module. */
    std::uint32_t value{};
};

/**
 * @brief Describes the basic type of a value produced by an IR instruction.
 *
 * These types are intentionally small and generic. They give later passes
 * enough information to understand common values without defining a full
 * language-specific type system here.
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
 * @brief Literal constant stored directly in an IR operand.
 *
 * A literal owns its value and can represent the primitive constants used by
 * the generic IR: integers, floating-point values, booleans and strings.
 * An empty literal is represented by std::monostate.
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
 * @brief Value consumed by an IR instruction.
 *
 * An operand can reference the result of another instruction, contain a
 * literal directly, or store a symbolic name. This keeps instructions generic
 * while still supporting several common kinds of input.
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
 * @brief Regular IR instruction stored inside a basic block.
 *
 * An instruction has an operation name, zero or more operands and an optional
 * result value. Instructions that do not produce a value keep result empty and
 * normally use ValueType::Void.
 */
struct Instruction {
    /** Operation name understood by the next lowering stage or backend. */
    std::string op{};

    /** Identifier of the produced value, or empty if the instruction has no result. */
    std::optional<ValueId> result{};

    /** Values consumed by the instruction. */
    std::vector<Operand> operands{};

    /** Type of the produced value. */
    ValueType type{ValueType::Unknown};
};

/**
 * @brief Instruction that ends a basic block.
 *
 * A terminator describes how execution leaves the block. Its targets can point
 * to other basic blocks, for example for jumps or conditional branches.
 */
struct Terminator {
    /** Terminator operation name, such as a branch or return operation. */
    std::string op{};

    /** Values consumed by the terminator. */
    std::vector<Operand> operands{};

    /** Basic blocks that can be reached from this terminator. */
    std::vector<BlockId> targets{};
};

/**
 * @brief Sequence of IR instructions forming one basic block.
 *
 * Instructions are executed in order. A block can end with one terminator,
 * which describes the next control-flow destination or the end of execution.
 */
struct BasicBlock {
    /** Identifier used to reference this block. */
    BlockId id{};

    /** Human-readable block name, mainly useful for debug output. */
    std::string name{};

    /** Regular instructions executed before the terminator. */
    std::vector<Instruction> instructions{};

    /** Optional instruction describing how control leaves the block. */
    std::optional<Terminator> terminator{};
};

/**
 * @brief Container for the high-level intermediate representation.
 *
 * HIR stays close to language-level operations while removing direct
 * dependency on the AST. It is the first IR produced by AST lowering.
 */
struct HIRModule {
    /** Basic blocks belonging to this HIR module. */
    std::vector<BasicBlock> blocks{};
};

/**
 * @brief Container for the medium-level intermediate representation.
 *
 * MIR is produced from HIR and is intended to contain simpler, more explicit
 * operations that are easier for later analysis or backend stages to consume.
 */
struct MIRModule {
    /** Basic blocks belonging to this MIR module. */
    std::vector<BasicBlock> blocks{};
};

/**
 * @brief Builds a HIR module one block and instruction at a time.
 *
 * A builder starts with an entry block, tracks the current block and assigns
 * new value identifiers automatically. Instructions are always emitted into
 * the current block.
 *
 * The builder owns the module while it is being created. Calling finish()
 * transfers the completed module to the caller.
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
     * Following calls to emit(), emitValue() and terminate() will operate on
     * this block until another block is selected.
     *
     * @param block Target block identifier.
     *
     * @throws std::runtime_error If the block identifier is invalid.
     */
    void setCurrentBlock(BlockId block);

    /**
     * @brief Emits an instruction that produces a value.
     *
     * A new ValueId is allocated automatically and stored as the result of the
     * instruction before it is appended to the current block.
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
     * The instruction is appended to the current block with no ValueId result
     * and uses ValueType::Void.
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
     * The terminator is stored separately from regular instructions and marks
     * the block as complete. No new instruction can be emitted into the block
     * after this call.
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
     * The internal module is moved out of the builder, so the returned value
     * becomes the completed HIR representation.
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
 * @brief Builds a MIR module one block and instruction at a time.
 *
 * The interface mirrors HIRBuilder so lowering code can create blocks, emit
 * instructions and add terminators in a predictable way. Result identifiers
 * are assigned automatically as new values are emitted.
 *
 * The builder owns the module while it is being created. Calling finish()
 * transfers the completed module to the caller.
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
     * Following calls to emit(), emitValue() and terminate() will operate on
     * this block until another block is selected.
     *
     * @param block Target block identifier.
     *
     * @throws std::runtime_error If the block identifier is invalid.
     */
    void setCurrentBlock(BlockId block);

    /**
     * @brief Emits an instruction that produces a value.
     *
     * A new ValueId is allocated automatically and stored as the result of the
     * instruction before it is appended to the current block.
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
     * The instruction is appended to the current block with no ValueId result
     * and uses ValueType::Void.
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
     * The terminator is stored separately from regular instructions and marks
     * the block as complete. No new instruction can be emitted into the block
     * after this call.
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
     * The internal module is moved out of the builder, so the returned value
     * becomes the completed MIR representation.
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
 * @brief Tracks value mappings while lowering HIR instructions to MIR.
 *
 * HIR and MIR use their own value identifiers. This context records which MIR
 * value replaces each HIR value so operands can be remapped while lowering
 * later instructions.
 */
class MIRLoweringContext {
public:
    /**
     * @brief Associates a HIR value with a MIR value.
     *
     * The mapping is later used to replace HIR value references when lowering
     * following instructions and terminators.
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
     * This is used when a later HIR instruction refers to a result that has
     * already been lowered to MIR.
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
     * Value operands are replaced with their mapped MIR ValueId. Literals and
     * symbolic operands are returned unchanged.
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
 * @brief Stores the functions used to lower AST nodes to HIR and HIR to MIR.
 *
 * Language features register their own lowering functions here. This keeps
 * the core IR pipeline independent from specific AST node kinds and operation
 * names.
 *
 * The registry also provides helpers for lowering child AST nodes and handles
 * HIR-to-MIR value mappings through MIRLoweringContext.
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
     * The node kind is used to find the registered lowering callback, which
     * can emit instructions into the provided HIRBuilder.
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
     * The instruction operation selects the registered MIR lowerer. If the HIR
     * instruction and the lowered MIR instruction both produce values, their
     * identifiers are automatically associated in the lowering context.
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
     * Node fields are inspected and any child node or list of child nodes is
     * forwarded to lowerHIR(). Field iteration order is the node storage order.
     *
     * @param node AST node whose child fields are lowered.
     * @param out HIR builder receiving emitted instructions.
     */
    void lowerChildren(const ast::Node &node, HIRBuilder &out) const;

    /**
     * @brief Lowers child nodes in schema field order when a schema is available.
     *
     * The node schema defines the traversal order, which makes lowering stable
     * even if the underlying field container has no meaningful order. If no
     * schema exists, the function falls back to direct field traversal.
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
 * @brief Runs the AST-to-HIR lowering stage.
 *
 * The pass creates a HIRBuilder and asks the LoweringRegistry to lower the
 * supplied AST root. The concrete behavior depends on the lowerers registered
 * by the active language features.
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
     * A fresh HIRBuilder is created, then the root node is lowered through the
     * registry. The finished builder becomes the returned HIR module.
     *
     * @param root AST root node.
     * @return Produced HIR module.
     */
    HIRModule lower(const ast::Node &root) const;

private:
    const LoweringRegistry &registry_;
};

/**
 * @brief Runs the HIR-to-MIR lowering stage.
 *
 * Blocks are visited in module order. Registered MIR lowerers translate each
 * instruction, while MIRLoweringContext keeps result references consistent
 * between both representations.
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
     * The pass recreates the HIR block structure in MIR, lowers each regular
     * instruction through the registry and remaps terminator operands through
     * MIRLoweringContext before emitting the terminator.
     *
     * @param hir HIR module to lower.
     * @return Produced MIR module.
     */
    MIRModule lower(const HIRModule &hir) const;

private:
    const LoweringRegistry &registry_;
};

/**
 * @brief Converts a ValueType to the textual name used by IR tools.
 *
 * The returned names are stable and suitable for debug output, diagnostics or
 * simple textual IR representations.
 *
 * @param type Value type to convert.
 * @return Textual type name.
 */
std::string valueTypeName(ValueType type);

} // namespace novac::ir