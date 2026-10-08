#include "novac/engine/transformation/IR.hpp"

#include <stdexcept>
#include <utility>
#include <variant>

namespace novac::ir {

/**
 * @brief Constructs a `Literal` instance.
 */
Literal::Literal()
    : data_{} {}

/**
 * @brief Constructs a `Literal` instance.
 *
 * @param data Value supplied for `data`.
 */
Literal::Literal(Data data)
    : data_{std::move(data)} {}

/**
 * @brief Implements the `integer` operation.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
Literal Literal::integer(int value) {
    return Literal{value};
}

/**
 * @brief Implements the `floating` operation.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
Literal Literal::floating(double value) {
    return Literal{value};
}

/**
 * @brief Implements the `boolean` operation.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
Literal Literal::boolean(bool value) {
    return Literal{value};
}

/**
 * @brief Implements the `string` operation.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
Literal Literal::string(std::string value) {
    return Literal{std::move(value)};
}

/**
 * @brief Implements the `toString` operation.
 *
 * @return Value produced by the operation.
 */
std::string Literal::toString() const {
    if (std::holds_alternative<std::monostate>(data_)) {
        return "none";
    }

    if (const auto *value{std::get_if<int>(&data_)}) {
        return std::to_string(*value);
    }

    if (const auto *value{std::get_if<double>(&data_)}) {
        return std::to_string(*value);
    }

    if (const auto *value{std::get_if<bool>(&data_)}) {
        return *value ? "true" : "false";
    }

    if (const auto *value{std::get_if<std::string>(&data_)}) {
        return *value;
    }

    return "<invalid>";
}

/**
 * @brief Constructs a `Operand` instance.
 *
 * @param data Value supplied for `data`.
 */
Operand::Operand(Data data)
    : data_{std::move(data)} {}

/**
 * @brief Implements the `fromValue` operation.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
Operand Operand::fromValue(ValueId id) {
    return Operand{id};
}

/**
 * @brief Implements the `fromLiteral` operation.
 *
 * @param literal Value supplied for `literal`.
 * @return Value produced by the operation.
 */
Operand Operand::fromLiteral(Literal literal) {
    return Operand{std::move(literal)};
}

/**
 * @brief Implements the `fromLiteral` operation.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
Operand Operand::fromLiteral(int value) {
    return fromLiteral(Literal::integer(value));
}

/**
 * @brief Implements the `fromLiteral` operation.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
Operand Operand::fromLiteral(double value) {
    return fromLiteral(Literal::floating(value));
}

/**
 * @brief Implements the `fromLiteral` operation.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
Operand Operand::fromLiteral(bool value) {
    return fromLiteral(Literal::boolean(value));
}

/**
 * @brief Implements the `fromLiteral` operation.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
Operand Operand::fromLiteral(std::string value) {
    return fromLiteral(Literal::string(std::move(value)));
}

/**
 * @brief Implements the `fromSymbol` operation.
 *
 * @param name Value supplied for `name`.
 * @return Value produced by the operation.
 */
Operand Operand::fromSymbol(std::string name) {
    return Operand{std::move(name)};
}

/**
 * @brief Checks the condition represented by `isValue`.
 *
 * @return Value produced by the operation.
 */
bool Operand::isValue() const {
    return std::holds_alternative<ValueId>(data_);
}

/**
 * @brief Implements the `asValue` operation.
 *
 * @return Value produced by the operation.
 */
ValueId Operand::asValue() const {
    if (const auto *value{std::get_if<ValueId>(&data_)}) {
        return *value;
    }

    throw std::runtime_error("Operand::asValue: operand is not a value id");
}

/**
 * @brief Implements the `data` operation.
 *
 * @return Value produced by the operation.
 */
const Operand::Data &Operand::data() const {
    return data_;
}

/**
 * @brief Implements the `toString` operation.
 *
 * @return Value produced by the operation.
 */
std::string Operand::toString() const {
    if (const auto *value{std::get_if<ValueId>(&data_)}) {
        return "v" + std::to_string(value->value);
    }

    if (const auto *literal{std::get_if<Literal>(&data_)}) {
        return literal->toString();
    }

    if (const auto *symbol{std::get_if<std::string>(&data_)}) {
        return *symbol;
    }

    return "<invalid>";
}

/**
 * @brief Constructs a `HIRBuilder` instance.
 */
HIRBuilder::HIRBuilder()
    : module_{}, currentBlock_{0}, nextValue_{0} {
    module_.blocks.push_back(BasicBlock{BlockId{0}, "entry", {}, {}});
}

/**
 * @brief Creates a value through `createBlock`.
 *
 * @param name Value supplied for `name`.
 * @return Value produced by the operation.
 */
BlockId HIRBuilder::createBlock(std::string name) {
    const BlockId id{static_cast<std::uint32_t>(module_.blocks.size())};

    module_.blocks.push_back(BasicBlock{id, std::move(name), {}, {}});

    return id;
}

/**
 * @brief Sets the value handled by `setCurrentBlock`.
 *
 * @param block Value supplied for `block`.
 */
void HIRBuilder::setCurrentBlock(BlockId block) {
    if (block.value >= module_.blocks.size()) {
        throw std::runtime_error("HIRBuilder::setCurrentBlock: invalid block id");
    }

    currentBlock_ = block;
}

/**
 * @brief Implements the `currentBlockId` operation.
 *
 * @return Value produced by the operation.
 */
BlockId HIRBuilder::currentBlockId() const noexcept {
    return currentBlock_;
}

/**
 * @brief Implements the `currentBlockTerminated` operation.
 *
 * @return Value produced by the operation.
 */
bool HIRBuilder::currentBlockTerminated() const {
    return currentBlock().terminator.has_value();
}

/**
 * @brief Emits output through `emitValue`.
 *
 * @param op Value supplied for `op`.
 * @param operands Value supplied for `operands`.
 * @param type Value supplied for `type`.
 * @return Value produced by the operation.
 */
ValueId HIRBuilder::emitValue(std::string op, std::vector<Operand> operands, ValueType type) {
    if (currentBlock().terminator.has_value()) {
        throw std::runtime_error("HIRBuilder::emitValue: cannot emit instruction after terminator");
    }

    const ValueId result{nextValue_++};

    currentBlock().instructions.push_back({std::move(op), result, std::move(operands), type});

    return result;
}

/**
 * @brief Emits output through `emitValue`.
 *
 * @param op Value supplied for `op`.
 * @param operands Value supplied for `operands`.
 * @param type Value supplied for `type`.
 * @return Value produced by the operation.
 */
ValueId HIRBuilder::emitValue(const ids::Operation &op, std::vector<Operand> operands, ValueType type) {
    return emitValue(op.value, std::move(operands), type);
}

/**
 * @brief Emits output through `emit`.
 *
 * @param op Value supplied for `op`.
 * @param operands Value supplied for `operands`.
 */
void HIRBuilder::emit(std::string op, std::vector<Operand> operands) {
    if (currentBlock().terminator.has_value()) {
        throw std::runtime_error("HIRBuilder::emit: cannot emit instruction after terminator");
    }

    currentBlock().instructions.push_back({std::move(op), std::nullopt, std::move(operands), ValueType::Void});
}

/**
 * @brief Emits output through `emit`.
 *
 * @param op Value supplied for `op`.
 * @param operands Value supplied for `operands`.
 */
void HIRBuilder::emit(const ids::Operation &op, std::vector<Operand> operands) {
    emit(op.value, std::move(operands));
}

/**
 * @brief Implements the `terminate` operation.
 *
 * @param op Value supplied for `op`.
 * @param operands Value supplied for `operands`.
 * @param targets Value supplied for `targets`.
 */
void HIRBuilder::terminate(std::string op, std::vector<Operand> operands, std::vector<BlockId> targets) {
    if (op.empty()) {
        throw std::runtime_error("HIRBuilder::terminate: terminator op cannot be empty");
    }

    if (currentBlock().terminator.has_value()) {
        throw std::runtime_error("HIRBuilder::terminate: block already has a terminator");
    }

    currentBlock().terminator = Terminator{std::move(op), std::move(operands), std::move(targets)};
}

/**
 * @brief Implements the `terminate` operation.
 *
 * @param op Value supplied for `op`.
 * @param operands Value supplied for `operands`.
 * @param targets Value supplied for `targets`.
 */
void HIRBuilder::terminate(const ids::Operation &op, std::vector<Operand> operands, std::vector<BlockId> targets) {
    terminate(op.value, std::move(operands), std::move(targets));
}

/**
 * @brief Implements the `finish` operation.
 *
 * @return Value produced by the operation.
 */
HIRModule HIRBuilder::finish() {
    return std::move(module_);
}

/**
 * @brief Implements the `currentBlock` operation.
 *
 * @return Value produced by the operation.
 */
BasicBlock &HIRBuilder::currentBlock() {
    if (currentBlock_.value >= module_.blocks.size()) {
        throw std::runtime_error("HIRBuilder::currentBlock: invalid current block");
    }

    return module_.blocks[currentBlock_.value];
}

/**
 * @brief Implements the `currentBlock` operation.
 *
 * @return Value produced by the operation.
 */
const BasicBlock &HIRBuilder::currentBlock() const {
    if (currentBlock_.value >= module_.blocks.size()) {
        throw std::runtime_error("HIRBuilder::currentBlock: invalid current block");
    }

    return module_.blocks[currentBlock_.value];
}

/**
 * @brief Constructs a `MIRBuilder` instance.
 */
MIRBuilder::MIRBuilder()
    : module_{}, currentBlock_{0}, nextValue_{0} {
    module_.blocks.push_back(BasicBlock{BlockId{0}, "entry", {}, {}});
}

/**
 * @brief Creates a value through `createBlock`.
 *
 * @param name Value supplied for `name`.
 * @return Value produced by the operation.
 */
BlockId MIRBuilder::createBlock(std::string name) {
    const BlockId id{static_cast<std::uint32_t>(module_.blocks.size())};

    module_.blocks.push_back(BasicBlock{id, std::move(name), {}, {}});

    return id;
}

/**
 * @brief Sets the value handled by `setCurrentBlock`.
 *
 * @param block Value supplied for `block`.
 */
void MIRBuilder::setCurrentBlock(BlockId block) {
    if (block.value >= module_.blocks.size()) {
        throw std::runtime_error("MIRBuilder::setCurrentBlock: invalid block id");
    }

    currentBlock_ = block;
}

/**
 * @brief Emits output through `emitValue`.
 *
 * @param op Value supplied for `op`.
 * @param operands Value supplied for `operands`.
 * @param type Value supplied for `type`.
 * @return Value produced by the operation.
 */
ValueId MIRBuilder::emitValue(std::string op, std::vector<Operand> operands, ValueType type) {
    if (currentBlock().terminator.has_value()) {
        throw std::runtime_error("MIRBuilder::emitValue: cannot emit instruction after terminator");
    }

    const ValueId result{nextValue_++};

    currentBlock().instructions.push_back({std::move(op), result, std::move(operands), type});

    return result;
}

/**
 * @brief Emits output through `emitValue`.
 *
 * @param op Value supplied for `op`.
 * @param operands Value supplied for `operands`.
 * @param type Value supplied for `type`.
 * @return Value produced by the operation.
 */
ValueId MIRBuilder::emitValue(const ids::Operation &op, std::vector<Operand> operands, ValueType type) {
    return emitValue(op.value, std::move(operands), type);
}

/**
 * @brief Emits output through `emit`.
 *
 * @param op Value supplied for `op`.
 * @param operands Value supplied for `operands`.
 */
void MIRBuilder::emit(std::string op, std::vector<Operand> operands) {
    if (currentBlock().terminator.has_value()) {
        throw std::runtime_error("MIRBuilder::emit: cannot emit instruction after terminator");
    }

    currentBlock().instructions.push_back({std::move(op), std::nullopt, std::move(operands), ValueType::Void});
}

/**
 * @brief Emits output through `emit`.
 *
 * @param op Value supplied for `op`.
 * @param operands Value supplied for `operands`.
 */
void MIRBuilder::emit(const ids::Operation &op, std::vector<Operand> operands) {
    emit(op.value, std::move(operands));
}

/**
 * @brief Implements the `terminate` operation.
 *
 * @param op Value supplied for `op`.
 * @param operands Value supplied for `operands`.
 * @param targets Value supplied for `targets`.
 */
void MIRBuilder::terminate(std::string op, std::vector<Operand> operands, std::vector<BlockId> targets) {
    if (op.empty()) {
        throw std::runtime_error("MIRBuilder::terminate: terminator op cannot be empty");
    }

    if (currentBlock().terminator.has_value()) {
        throw std::runtime_error("MIRBuilder::terminate: block already has a terminator");
    }

    currentBlock().terminator = Terminator{std::move(op), std::move(operands), std::move(targets)};
}

/**
 * @brief Implements the `terminate` operation.
 *
 * @param op Value supplied for `op`.
 * @param operands Value supplied for `operands`.
 * @param targets Value supplied for `targets`.
 */
void MIRBuilder::terminate(const ids::Operation &op, std::vector<Operand> operands, std::vector<BlockId> targets) {
    terminate(op.value, std::move(operands), std::move(targets));
}

/**
 * @brief Implements the `finish` operation.
 *
 * @return Value produced by the operation.
 */
MIRModule MIRBuilder::finish() {
    return std::move(module_);
}

/**
 * @brief Implements the `currentBlock` operation.
 *
 * @return Value produced by the operation.
 */
BasicBlock &MIRBuilder::currentBlock() {
    if (currentBlock_.value >= module_.blocks.size()) {
        throw std::runtime_error("MIRBuilder::currentBlock: invalid current block");
    }

    return module_.blocks[currentBlock_.value];
}

/**
 * @brief Implements the `currentBlock` operation.
 *
 * @return Value produced by the operation.
 */
const BasicBlock &MIRBuilder::currentBlock() const {
    if (currentBlock_.value >= module_.blocks.size()) {
        throw std::runtime_error("MIRBuilder::currentBlock: invalid current block");
    }

    return module_.blocks[currentBlock_.value];
}

/**
 * @brief Implements the `bind` operation.
 *
 * @param hirValue Value supplied for `hirValue`.
 * @param mirValue Value supplied for `mirValue`.
 */
void MIRLoweringContext::bind(ValueId hirValue, ValueId mirValue) {
    values_[hirValue.value] = mirValue;
}

/**
 * @brief Checks the condition represented by `has`.
 *
 * @param hirValue Value supplied for `hirValue`.
 * @return Value produced by the operation.
 */
bool MIRLoweringContext::has(ValueId hirValue) const {
    return values_.find(hirValue.value) != values_.end();
}

/**
 * @brief Resolves data through `resolve`.
 *
 * @param hirValue Value supplied for `hirValue`.
 * @return Value produced by the operation.
 */
ValueId MIRLoweringContext::resolve(ValueId hirValue) const {
    const auto iter{values_.find(hirValue.value)};

    if (iter == values_.end()) {
        throw std::runtime_error("MIRLoweringContext::resolve: unknown HIR value v" + std::to_string(hirValue.value));
    }

    return iter->second;
}

/**
 * @brief Implements the `remapOperand` operation.
 *
 * @param operand Value supplied for `operand`.
 * @return Value produced by the operation.
 */
Operand MIRLoweringContext::remapOperand(const Operand &operand) const {
    if (!operand.isValue()) {
        return operand;
    }

    return Operand::fromValue(resolve(operand.asValue()));
}

/**
 * @brief Implements the `remapOperands` operation.
 *
 * @param operands Value supplied for `operands`.
 * @return Value produced by the operation.
 */
std::vector<Operand> MIRLoweringContext::remapOperands(const std::vector<Operand> &operands) const {
    std::vector<Operand> remapped{};
    remapped.reserve(operands.size());

    for (const Operand &operand : operands) {
        remapped.push_back(remapOperand(operand));
    }

    return remapped;
}

/**
 * @brief Constructs a `LoweringRegistry` instance.
 *
 * @param duplicatePolicy Value supplied for `duplicatePolicy`.
 */
LoweringRegistry::LoweringRegistry(registry::DuplicatePolicy duplicatePolicy)
    : hir_{}, mir_{}, duplicatePolicy_{duplicatePolicy} {}

/**
 * @brief Implements the `hir` operation.
 *
 * @param nodeKind Value supplied for `nodeKind`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus LoweringRegistry::hir(std::string nodeKind, HIRLowerer fn) {
    if (nodeKind.empty()) {
        throw std::runtime_error("LoweringRegistry::hir: node kind cannot be empty");
    }

    if (!fn) {
        throw std::runtime_error("LoweringRegistry::hir: lowerer cannot be empty");
    }

    return registry::registerEntry(hir_, std::move(nodeKind), std::move(fn), duplicatePolicy_, "LoweringRegistry::hir");
}

/**
 * @brief Implements the `hir` operation.
 *
 * @param nodeKind Value supplied for `nodeKind`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus LoweringRegistry::hir(const ids::NodeKind &nodeKind, HIRLowerer fn) {
    return hir(nodeKind.value, std::move(fn));
}

/**
 * @brief Implements the `mir` operation.
 *
 * @param hirKind Value supplied for `hirKind`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus LoweringRegistry::mir(std::string hirKind, MIRLowerer fn) {
    if (hirKind.empty()) {
        throw std::runtime_error("LoweringRegistry::mir: HIR kind cannot be empty");
    }

    if (!fn) {
        throw std::runtime_error("LoweringRegistry::mir: lowerer cannot be empty");
    }

    return registry::registerEntry(mir_, std::move(hirKind), std::move(fn), duplicatePolicy_, "LoweringRegistry::mir");
}

/**
 * @brief Implements the `mir` operation.
 *
 * @param hirKind Value supplied for `hirKind`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus LoweringRegistry::mir(const ids::Operation &hirKind, MIRLowerer fn) {
    return mir(hirKind.value, std::move(fn));
}

/**
 * @brief Checks the condition represented by `hasHIR`.
 *
 * @param nodeKind Value supplied for `nodeKind`.
 * @return Value produced by the operation.
 */
bool LoweringRegistry::hasHIR(const std::string &nodeKind) const {
    return hir_.find(nodeKind) != hir_.end();
}

/**
 * @brief Checks the condition represented by `hasHIR`.
 *
 * @param nodeKind Value supplied for `nodeKind`.
 * @return Value produced by the operation.
 */
bool LoweringRegistry::hasHIR(const ids::NodeKind &nodeKind) const {
    return hasHIR(nodeKind.value);
}

/**
 * @brief Checks the condition represented by `hasMIR`.
 *
 * @param hirKind Value supplied for `hirKind`.
 * @return Value produced by the operation.
 */
bool LoweringRegistry::hasMIR(const std::string &hirKind) const {
    return mir_.find(hirKind) != mir_.end();
}

/**
 * @brief Checks the condition represented by `hasMIR`.
 *
 * @param hirKind Value supplied for `hirKind`.
 * @return Value produced by the operation.
 */
bool LoweringRegistry::hasMIR(const ids::Operation &hirKind) const {
    return hasMIR(hirKind.value);
}

/**
 * @brief Lowers input through `lowerHIR`.
 *
 * @param node Value supplied for `node`.
 * @param out Value supplied for `out`.
 * @return Value produced by the operation.
 */
LoweringRegistry::LoweredValue LoweringRegistry::lowerHIR(const ast::Node &node, HIRBuilder &out) const {
    const auto iter{hir_.find(node.kind())};

    if (iter == hir_.end()) {
        throw std::runtime_error("LoweringRegistry::lowerHIR: missing HIR lowerer for AST node kind '" + node.kind() + "'");
    }

    return iter->second(node, out, *this);
}

/**
 * @brief Lowers input through `lowerMIR`.
 *
 * @param instruction Value supplied for `instruction`.
 * @param out Value supplied for `out`.
 * @param context Value supplied for `context`.
 * @return Value produced by the operation.
 */
LoweringRegistry::LoweredValue LoweringRegistry::lowerMIR(const Instruction &instruction, MIRBuilder &out, MIRLoweringContext &context) const {
    const auto iter{mir_.find(instruction.op)};

    if (iter == mir_.end()) {
        throw std::runtime_error("LoweringRegistry::lowerMIR: missing MIR lowerer for HIR instruction '" + instruction.op + "'");
    }

    LoweredValue lowered{iter->second(instruction, out, context, *this)};

    if (instruction.result.has_value() && lowered.has_value()) {
        context.bind(*instruction.result, *lowered);
    }

    return lowered;
}

/**
 * @brief Lowers input through `lowerChildren`.
 *
 * @param node Value supplied for `node`.
 * @param out Value supplied for `out`.
 */
void LoweringRegistry::lowerChildren(const ast::Node &node, HIRBuilder &out) const {
    for (const auto &[name, field] : node.fields()) {
        static_cast<void>(name);

        if (const auto *child{std::get_if<ast::NodePtr>(&field)}; child && *child) {
            lowerHIR(**child, out);
        }

        if (const auto *list{std::get_if<ast::NodeList>(&field)}) {
            for (const ast::NodePtr &child : *list) {
                if (child) {
                    lowerHIR(*child, out);
                }
            }
        }
    }
}

/**
 * @brief Lowers input through `lowerChildren`.
 *
 * @param node Value supplied for `node`.
 * @param nodes Value supplied for `nodes`.
 * @param out Value supplied for `out`.
 */
void LoweringRegistry::lowerChildren(const ast::Node &node, const ast::NodeRegistry &nodes, HIRBuilder &out) const {
    const ast::NodeSchema *schema{nodes.find(node.kind())};

    if (!schema) {
        lowerChildren(node, out);
        return;
    }

    for (const ast::FieldSchema &fieldSchema : schema->fields) {
        if (!node.has(fieldSchema.name)) {
            continue;
        }

        const ast::Field &field{node.field(fieldSchema.name)};

        if (const auto *child{std::get_if<ast::NodePtr>(&field)}; child && *child) {
            lowerHIR(**child, out);
        }

        if (const auto *list{std::get_if<ast::NodeList>(&field)}) {
            for (const ast::NodePtr &child : *list) {
                if (child) {
                    lowerHIR(*child, out);
                }
            }
        }
    }
}

/**
 * @brief Constructs a `ASTLoweringPass` instance.
 *
 * @param registry Value supplied for `registry`.
 */
ASTLoweringPass::ASTLoweringPass(const LoweringRegistry &registry)
    : registry_{registry} {}

/**
 * @brief Lowers input through `lower`.
 *
 * @param root Value supplied for `root`.
 * @return Value produced by the operation.
 */
HIRModule ASTLoweringPass::lower(const ast::Node &root) const {
    HIRBuilder builder{};

    registry_.lowerHIR(root, builder);

    return builder.finish();
}

/**
 * @brief Constructs a `HIRLoweringPass` instance.
 *
 * @param registry Value supplied for `registry`.
 */
HIRLoweringPass::HIRLoweringPass(const LoweringRegistry &registry)
    : registry_{registry} {}

/**
 * @brief Lowers input through `lower`.
 *
 * @param hir Value supplied for `hir`.
 * @return Value produced by the operation.
 */
MIRModule HIRLoweringPass::lower(const HIRModule &hir) const {
    MIRBuilder builder{};
    MIRLoweringContext context{};

    for (const BasicBlock &block : hir.blocks) {
        if (block.id.value != 0) {
            builder.createBlock(block.name);
        }

        builder.setCurrentBlock(block.id);

        for (const Instruction &instruction : block.instructions) {
            registry_.lowerMIR(instruction, builder, context);
        }

        if (block.terminator.has_value()) {
            builder.terminate(
                block.terminator->op,
                context.remapOperands(block.terminator->operands),
                block.terminator->targets);
        }
    }

    return builder.finish();
}

/**
 * @brief Implements the `valueTypeName` operation.
 *
 * @param type Value supplied for `type`.
 * @return Value produced by the operation.
 */
std::string valueTypeName(ValueType type) {
    switch (type) {
        case ValueType::Unknown: return "unknown";
        case ValueType::Void: return "void";
        case ValueType::Int: return "int";
        case ValueType::Bool: return "bool";
        case ValueType::Float: return "float";
        case ValueType::String: return "string";
    }

    return "unknown";
}

} // namespace novac::ir
