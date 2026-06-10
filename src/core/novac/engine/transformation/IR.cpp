#include "novac/engine/transformation/IR.hpp"

#include <stdexcept>
#include <utility>
#include <variant>

namespace novac::ir {

Literal::Literal()
    : data_{} {}

Literal::Literal(Data data)
    : data_{std::move(data)} {}

Literal Literal::integer(int value) {
    return Literal{value};
}

Literal Literal::floating(double value) {
    return Literal{value};
}

Literal Literal::boolean(bool value) {
    return Literal{value};
}

Literal Literal::string(std::string value) {
    return Literal{std::move(value)};
}

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

Operand::Operand(Data data)
    : data_{std::move(data)} {}

Operand Operand::fromValue(ValueId id) {
    return Operand{id};
}

Operand Operand::fromLiteral(Literal literal) {
    return Operand{std::move(literal)};
}

Operand Operand::fromLiteral(int value) {
    return fromLiteral(Literal::integer(value));
}

Operand Operand::fromLiteral(double value) {
    return fromLiteral(Literal::floating(value));
}

Operand Operand::fromLiteral(bool value) {
    return fromLiteral(Literal::boolean(value));
}

Operand Operand::fromLiteral(std::string value) {
    return fromLiteral(Literal::string(std::move(value)));
}

Operand Operand::fromSymbol(std::string name) {
    return Operand{std::move(name)};
}

bool Operand::isValue() const {
    return std::holds_alternative<ValueId>(data_);
}

ValueId Operand::asValue() const {
    if (const auto *value{std::get_if<ValueId>(&data_)}) {
        return *value;
    }

    throw std::runtime_error("Operand::asValue: operand is not a value id");
}

const Operand::Data &Operand::data() const {
    return data_;
}

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

HIRBuilder::HIRBuilder()
    : module_{}, currentBlock_{0}, nextValue_{0} {
    module_.blocks.push_back(BasicBlock{BlockId{0}, "entry", {}, {}});
}

BlockId HIRBuilder::createBlock(std::string name) {
    const BlockId id{static_cast<std::uint32_t>(module_.blocks.size())};

    module_.blocks.push_back(BasicBlock{id, std::move(name), {}, {}});

    return id;
}

void HIRBuilder::setCurrentBlock(BlockId block) {
    if (block.value >= module_.blocks.size()) {
        throw std::runtime_error("HIRBuilder::setCurrentBlock: invalid block id");
    }

    currentBlock_ = block;
}

ValueId HIRBuilder::emitValue(std::string op, std::vector<Operand> operands, ValueType type) {
    if (currentBlock().terminator.has_value()) {
        throw std::runtime_error("HIRBuilder::emitValue: cannot emit instruction after terminator");
    }

    const ValueId result{nextValue_++};

    currentBlock().instructions.push_back({std::move(op), result, std::move(operands), type});

    return result;
}

ValueId HIRBuilder::emitValue(const ids::Operation &op, std::vector<Operand> operands, ValueType type) {
    return emitValue(op.value, std::move(operands), type);
}

void HIRBuilder::emit(std::string op, std::vector<Operand> operands) {
    if (currentBlock().terminator.has_value()) {
        throw std::runtime_error("HIRBuilder::emit: cannot emit instruction after terminator");
    }

    currentBlock().instructions.push_back({std::move(op), std::nullopt, std::move(operands), ValueType::Void});
}

void HIRBuilder::emit(const ids::Operation &op, std::vector<Operand> operands) {
    emit(op.value, std::move(operands));
}

void HIRBuilder::terminate(std::string op, std::vector<Operand> operands, std::vector<BlockId> targets) {
    if (op.empty()) {
        throw std::runtime_error("HIRBuilder::terminate: terminator op cannot be empty");
    }

    if (currentBlock().terminator.has_value()) {
        throw std::runtime_error("HIRBuilder::terminate: block already has a terminator");
    }

    currentBlock().terminator = Terminator{std::move(op), std::move(operands), std::move(targets)};
}

void HIRBuilder::terminate(const ids::Operation &op, std::vector<Operand> operands, std::vector<BlockId> targets) {
    terminate(op.value, std::move(operands), std::move(targets));
}

HIRModule HIRBuilder::finish() {
    return std::move(module_);
}

BasicBlock &HIRBuilder::currentBlock() {
    if (currentBlock_.value >= module_.blocks.size()) {
        throw std::runtime_error("HIRBuilder::currentBlock: invalid current block");
    }

    return module_.blocks[currentBlock_.value];
}

const BasicBlock &HIRBuilder::currentBlock() const {
    if (currentBlock_.value >= module_.blocks.size()) {
        throw std::runtime_error("HIRBuilder::currentBlock: invalid current block");
    }

    return module_.blocks[currentBlock_.value];
}

MIRBuilder::MIRBuilder()
    : module_{}, currentBlock_{0}, nextValue_{0} {
    module_.blocks.push_back(BasicBlock{BlockId{0}, "entry", {}, {}});
}

BlockId MIRBuilder::createBlock(std::string name) {
    const BlockId id{static_cast<std::uint32_t>(module_.blocks.size())};

    module_.blocks.push_back(BasicBlock{id, std::move(name), {}, {}});

    return id;
}

void MIRBuilder::setCurrentBlock(BlockId block) {
    if (block.value >= module_.blocks.size()) {
        throw std::runtime_error("MIRBuilder::setCurrentBlock: invalid block id");
    }

    currentBlock_ = block;
}

ValueId MIRBuilder::emitValue(std::string op, std::vector<Operand> operands, ValueType type) {
    if (currentBlock().terminator.has_value()) {
        throw std::runtime_error("MIRBuilder::emitValue: cannot emit instruction after terminator");
    }

    const ValueId result{nextValue_++};

    currentBlock().instructions.push_back({std::move(op), result, std::move(operands), type});

    return result;
}

ValueId MIRBuilder::emitValue(const ids::Operation &op, std::vector<Operand> operands, ValueType type) {
    return emitValue(op.value, std::move(operands), type);
}

void MIRBuilder::emit(std::string op, std::vector<Operand> operands) {
    if (currentBlock().terminator.has_value()) {
        throw std::runtime_error("MIRBuilder::emit: cannot emit instruction after terminator");
    }

    currentBlock().instructions.push_back({std::move(op), std::nullopt, std::move(operands), ValueType::Void});
}

void MIRBuilder::emit(const ids::Operation &op, std::vector<Operand> operands) {
    emit(op.value, std::move(operands));
}

void MIRBuilder::terminate(std::string op, std::vector<Operand> operands, std::vector<BlockId> targets) {
    if (op.empty()) {
        throw std::runtime_error("MIRBuilder::terminate: terminator op cannot be empty");
    }

    if (currentBlock().terminator.has_value()) {
        throw std::runtime_error("MIRBuilder::terminate: block already has a terminator");
    }

    currentBlock().terminator = Terminator{std::move(op), std::move(operands), std::move(targets)};
}

void MIRBuilder::terminate(const ids::Operation &op, std::vector<Operand> operands, std::vector<BlockId> targets) {
    terminate(op.value, std::move(operands), std::move(targets));
}

MIRModule MIRBuilder::finish() {
    return std::move(module_);
}

BasicBlock &MIRBuilder::currentBlock() {
    if (currentBlock_.value >= module_.blocks.size()) {
        throw std::runtime_error("MIRBuilder::currentBlock: invalid current block");
    }

    return module_.blocks[currentBlock_.value];
}

const BasicBlock &MIRBuilder::currentBlock() const {
    if (currentBlock_.value >= module_.blocks.size()) {
        throw std::runtime_error("MIRBuilder::currentBlock: invalid current block");
    }

    return module_.blocks[currentBlock_.value];
}

void MIRLoweringContext::bind(ValueId hirValue, ValueId mirValue) {
    values_[hirValue.value] = mirValue;
}

bool MIRLoweringContext::has(ValueId hirValue) const {
    return values_.find(hirValue.value) != values_.end();
}

ValueId MIRLoweringContext::resolve(ValueId hirValue) const {
    const auto iter{values_.find(hirValue.value)};

    if (iter == values_.end()) {
        throw std::runtime_error("MIRLoweringContext::resolve: unknown HIR value v" + std::to_string(hirValue.value));
    }

    return iter->second;
}

Operand MIRLoweringContext::remapOperand(const Operand &operand) const {
    if (!operand.isValue()) {
        return operand;
    }

    return Operand::fromValue(resolve(operand.asValue()));
}

std::vector<Operand> MIRLoweringContext::remapOperands(const std::vector<Operand> &operands) const {
    std::vector<Operand> remapped{};
    remapped.reserve(operands.size());

    for (const Operand &operand : operands) {
        remapped.push_back(remapOperand(operand));
    }

    return remapped;
}

LoweringRegistry::LoweringRegistry(registry::DuplicatePolicy duplicatePolicy)
    : hir_{}, mir_{}, duplicatePolicy_{duplicatePolicy} {}

registry::RegisterStatus LoweringRegistry::hir(std::string nodeKind, HIRLowerer fn) {
    if (nodeKind.empty()) {
        throw std::runtime_error("LoweringRegistry::hir: node kind cannot be empty");
    }

    if (!fn) {
        throw std::runtime_error("LoweringRegistry::hir: lowerer cannot be empty");
    }

    return registry::registerEntry(hir_, std::move(nodeKind), std::move(fn), duplicatePolicy_, "LoweringRegistry::hir");
}

registry::RegisterStatus LoweringRegistry::hir(const ids::NodeKind &nodeKind, HIRLowerer fn) {
    return hir(nodeKind.value, std::move(fn));
}

registry::RegisterStatus LoweringRegistry::mir(std::string hirKind, MIRLowerer fn) {
    if (hirKind.empty()) {
        throw std::runtime_error("LoweringRegistry::mir: HIR kind cannot be empty");
    }

    if (!fn) {
        throw std::runtime_error("LoweringRegistry::mir: lowerer cannot be empty");
    }

    return registry::registerEntry(mir_, std::move(hirKind), std::move(fn), duplicatePolicy_, "LoweringRegistry::mir");
}

registry::RegisterStatus LoweringRegistry::mir(const ids::Operation &hirKind, MIRLowerer fn) {
    return mir(hirKind.value, std::move(fn));
}

bool LoweringRegistry::hasHIR(const std::string &nodeKind) const {
    return hir_.find(nodeKind) != hir_.end();
}

bool LoweringRegistry::hasHIR(const ids::NodeKind &nodeKind) const {
    return hasHIR(nodeKind.value);
}

bool LoweringRegistry::hasMIR(const std::string &hirKind) const {
    return mir_.find(hirKind) != mir_.end();
}

bool LoweringRegistry::hasMIR(const ids::Operation &hirKind) const {
    return hasMIR(hirKind.value);
}

LoweringRegistry::LoweredValue LoweringRegistry::lowerHIR(const ast::Node &node, HIRBuilder &out) const {
    const auto iter{hir_.find(node.kind())};

    if (iter == hir_.end()) {
        throw std::runtime_error("LoweringRegistry::lowerHIR: missing HIR lowerer for AST node kind '" + node.kind() + "'");
    }

    return iter->second(node, out, *this);
}

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

ASTLoweringPass::ASTLoweringPass(const LoweringRegistry &registry)
    : registry_{registry} {}

HIRModule ASTLoweringPass::lower(const ast::Node &root) const {
    HIRBuilder builder{};

    registry_.lowerHIR(root, builder);

    return builder.finish();
}

HIRLoweringPass::HIRLoweringPass(const LoweringRegistry &registry)
    : registry_{registry} {}

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