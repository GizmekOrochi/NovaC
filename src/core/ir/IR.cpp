#include "../../include/ir/IR.hpp"

#include "../../include/registry/RegistryHelpers.hpp"

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

    if (const auto *value{std::get_if<int>(&data_)}) return std::to_string(*value);
    if (const auto *value{std::get_if<double>(&data_)}) return std::to_string(*value);
    if (const auto *value{std::get_if<bool>(&data_)}) return *value ? "true" : "false";
    if (const auto *value{std::get_if<std::string>(&data_)}) return *value;

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

std::string Operand::toString() const {
    if (const auto *valueId{std::get_if<ValueId>(&data_)}) return "v" + std::to_string(valueId->value);
    if (const auto *literal{std::get_if<Literal>(&data_)}) return literal->toString();
    if (const auto *symbol{std::get_if<std::string>(&data_)})return *symbol;

    return "<invalid>";
}

HIRBuilder::HIRBuilder()
    : module_{}, currentBlock_{0}, nextValue_{0} {
    module_.blocks.push_back(BasicBlock{BlockId{0}, "entry", {}});
}

BlockId HIRBuilder::createBlock(std::string name) {
    const BlockId id{static_cast<std::uint32_t>(module_.blocks.size())};

    module_.blocks.push_back(BasicBlock{id, std::move(name), {}});

    return id;
}

void HIRBuilder::setCurrentBlock(BlockId block) {
    if (block.value >= module_.blocks.size()) {
        throw std::runtime_error("HIRBuilder::setCurrentBlock: invalid block id");
    }

    currentBlock_ = block;
}

ValueId HIRBuilder::emitValue(std::string op, std::vector<Operand> operands, types::TypeRef type) {
    const ValueId result{nextValue_++};

    currentBlock().instructions.push_back({std::move(op), result, std::move(operands), std::move(type)});

    return result;
}

ValueId HIRBuilder::emitValue(const ids::Operation &op, std::vector<Operand> operands, types::TypeRef type) {
    return emitValue(op.value, std::move(operands), std::move(type));
}

void HIRBuilder::emit(std::string op, std::vector<Operand> operands) {
    currentBlock().instructions.push_back({std::move(op), std::nullopt, std::move(operands), nullptr});
}

void HIRBuilder::emit(const ids::Operation &op, std::vector<Operand> operands) {
    emit(op.value, std::move(operands));
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

MIRBuilder::MIRBuilder()
    : module_{}, currentBlock_{0}, nextValue_{0} {
    module_.blocks.push_back(BasicBlock{BlockId{0}, "entry", {}});
}

BlockId MIRBuilder::createBlock(std::string name) {
    const BlockId id{static_cast<std::uint32_t>(module_.blocks.size())};

    module_.blocks.push_back(BasicBlock{id, std::move(name), {}});

    return id;
}

void MIRBuilder::setCurrentBlock(BlockId block) {
    if (block.value >= module_.blocks.size()) {
        throw std::runtime_error("MIRBuilder::setCurrentBlock: invalid block id");
    }

    currentBlock_ = block;
}

ValueId MIRBuilder::emitValue(std::string op, std::vector<Operand> operands, types::TypeRef type) {
    const ValueId result{nextValue_++};

    currentBlock().instructions.push_back({std::move(op), result, std::move(operands), std::move(type)});

    return result;
}

ValueId MIRBuilder::emitValue(const ids::Operation &op, std::vector<Operand> operands, types::TypeRef type) {
    return emitValue(op.value, std::move(operands), std::move(type));
}

void MIRBuilder::emit(std::string op, std::vector<Operand> operands) {
    currentBlock().instructions.push_back({std::move(op), std::nullopt, std::move(operands), nullptr});
}

void MIRBuilder::emit(const ids::Operation &op, std::vector<Operand> operands) {
    emit(op.value, std::move(operands));
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

LoweringRegistry::LoweringRegistry(registry::DuplicatePolicy duplicatePolicy)
    : hir_{}, mir_{}, duplicatePolicy_{duplicatePolicy} {
}

registry::RegisterStatus LoweringRegistry::hir(std::string nodeKind, HIRLowerer fn) {
    return registry::registerEntry(hir_, std::move(nodeKind), std::move(fn), duplicatePolicy_, "LoweringRegistry::hir");
}

registry::RegisterStatus LoweringRegistry::hir(const ids::NodeKind &nodeKind, HIRLowerer fn) {
    return hir(nodeKind.value, std::move(fn));
}

registry::RegisterStatus LoweringRegistry::mir(std::string hirKind, MIRLowerer fn) {
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

void LoweringRegistry::lowerHIR(const ast::Node &node, HIRBuilder &out) const {
    const auto iter{hir_.find(node.kind())};

    if (iter == hir_.end()) {
        throw std::runtime_error("LoweringRegistry::lowerHIR: missing HIR lowerer for AST node kind '" + node.kind() + "'");
    }

    iter->second(node, out, *this);
}

void LoweringRegistry::lowerMIR(const Instruction &instruction, MIRBuilder &out) const {
    const auto iter{mir_.find(instruction.op)};

    if (iter == mir_.end()) {
        throw std::runtime_error("LoweringRegistry::lowerMIR: missing MIR lowerer for HIR instruction '" + instruction.op + "'");
    }

    iter->second(instruction, out, *this);
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

ASTLoweringPass::ASTLoweringPass(const LoweringRegistry &registry)
    : registry_{registry} {}

HIRModule ASTLoweringPass::lower(const ast::Node &root) const {
    HIRBuilder builder{};

    registry_.lowerHIR(root, builder);

    return builder.finish();
}

HIRLoweringPass::HIRLoweringPass(const LoweringRegistry &registry)
    : registry_{registry} { }

MIRModule HIRLoweringPass::lower(const HIRModule &hir) const {
    MIRBuilder builder{};

    for (const BasicBlock &block : hir.blocks) {
        if (block.id.value != 0) {
            builder.createBlock(block.name);
        }

        builder.setCurrentBlock(block.id);

        for (const Instruction &instruction : block.instructions) {
            registry_.lowerMIR(instruction, builder);
        }
    }

    return builder.finish();
}

} // namespace novac::ir