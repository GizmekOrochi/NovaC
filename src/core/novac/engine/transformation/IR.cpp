#include "novac/engine/transformation/IR.hpp"

#include <limits>
#include <stdexcept>
#include <utility>
#include <variant>

namespace novac::ir {

namespace {

struct ValueDefinition {
    std::size_t block{};
    std::size_t instruction{};
};

void validateBlocks(const std::vector<BasicBlock> &blocks, const char *kind, IRValidationOptions options) {
    const std::string prefix{std::string(kind) + " validation: "};
    if (blocks.empty()) throw std::runtime_error(prefix + "module has no entry block");
    if (blocks.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw std::runtime_error(prefix + "too many blocks");
    }

    std::unordered_map<std::uint32_t, ValueDefinition> definitions;
    std::vector<std::vector<std::size_t>> successors(blocks.size());
    std::vector<std::vector<std::size_t>> predecessors(blocks.size());

    for (std::size_t b = 0; b < blocks.size(); ++b) {
        const BasicBlock &block = blocks[b];
        const std::string where{prefix + "block " + std::to_string(b) + ": "};
        if (block.id.value != b) throw std::runtime_error(where + "block ID does not match its index");
        if (options.requireTerminators && !block.terminator) {
            throw std::runtime_error(where + "missing terminator");
        }
        for (std::size_t i = 0; i < block.instructions.size(); ++i) {
            const Instruction &instruction = block.instructions[i];
            if (instruction.op.empty()) {
                throw std::runtime_error(where + "instruction " + std::to_string(i) + " has empty operation");
            }
            if (instruction.result && instruction.type == ValueType::Void) {
                throw std::runtime_error(where + "value-producing instruction has void type");
            }
            if (!instruction.result && instruction.type != ValueType::Void) {
                throw std::runtime_error(where + "instruction without result must have void type");
            }
            if (instruction.result && !definitions.emplace(instruction.result->value, ValueDefinition{b, i}).second) {
                throw std::runtime_error(where + "duplicate value v" + std::to_string(instruction.result->value));
            }
        }
        if (!block.terminator) continue;
        const Terminator &term = *block.terminator;
        if (term.op.empty()) throw std::runtime_error(where + "empty terminator operation");
        // These operations have engine-defined CFG semantics; extension-defined
        // terminators retain their own operation-specific successor contracts.
        if ((term.op == "jump" && term.targets.size() != 1) ||
            (term.op == "branch" && term.targets.size() != 2) ||
            (term.op == "return" && !term.targets.empty())) {
            throw std::runtime_error(where + "invalid successor count for '" + term.op + "'");
        }
        for (BlockId target : term.targets) {
            if (target.value >= blocks.size()) {
                throw std::runtime_error(where + "invalid target block " + std::to_string(target.value));
            }
            successors[b].push_back(target.value);
            predecessors[target.value].push_back(b);
        }
    }

    // Reachable CFG and iterative dominators. Non-reachable blocks may still
    // contain local definitions, but may not consume values from other blocks.
    std::vector<bool> reachable(blocks.size(), false);
    reachable[0] = true;
    std::vector<std::size_t> worklist{0};
    for (std::size_t cursor = 0; cursor < worklist.size(); ++cursor) {
        for (const auto to : successors[worklist[cursor]]) {
            if (!reachable[to]) { reachable[to] = true; worklist.push_back(to); }
        }
    }
    std::vector<std::vector<bool>> dom(blocks.size(), std::vector<bool>(blocks.size(), false));
    for (std::size_t b = 0; b < blocks.size(); ++b) {
        if (reachable[b]) {
            for (std::size_t d = 0; d < blocks.size(); ++d) dom[b][d] = reachable[d];
        }
    }
    dom[0].assign(blocks.size(), false);
    dom[0][0] = true;
    bool changed = true;
    while (changed) {
        changed = false;
        for (std::size_t b = 1; b < blocks.size(); ++b) {
            if (!reachable[b]) continue;
            std::vector<bool> next(blocks.size(), true);
            for (std::size_t pred : predecessors[b]) {
                if (!reachable[pred]) continue;
                for (std::size_t d = 0; d < blocks.size(); ++d) next[d] = next[d] && dom[pred][d];
            }
            next[b] = true;
            if (next != dom[b]) { dom[b] = std::move(next); changed = true; }
        }
    }

    const auto checkOperands = [&](const std::vector<Operand> &operands, std::size_t block, std::size_t instruction, const std::string &where) {
        for (const Operand &operand : operands) {
            if (!operand.isValue()) continue; // Literal/symbol types are language-defined.
            const std::uint32_t v = operand.asValue().value;
            const auto it = definitions.find(v);
            if (it == definitions.end()) throw std::runtime_error(where + "undefined value v" + std::to_string(v));
            const ValueDefinition def = it->second;
            if (def.block == block) {
                if (def.instruction >= instruction) {
                    throw std::runtime_error(where + "use of v" + std::to_string(v) + " before its definition");
                }
            } else if (!reachable[block] || !dom[block][def.block]) {
                throw std::runtime_error(where + "definition of v" + std::to_string(v) + " does not dominate its use");
            }
        }
    };
    for (std::size_t b = 0; b < blocks.size(); ++b) {
        const BasicBlock &block = blocks[b];
        for (std::size_t i = 0; i < block.instructions.size(); ++i) {
            checkOperands(block.instructions[i].operands, b, i,
                prefix + "block " + std::to_string(b) + " instruction " + std::to_string(i) + ": ");
        }
        if (block.terminator) {
            checkOperands(block.terminator->operands, b, block.instructions.size(),
                prefix + "block " + std::to_string(b) + " terminator: ");
        }
    }
}

void checkTargets(const std::vector<BlockId> &targets, std::size_t size, const char *owner) {
    for (BlockId target : targets) {
        if (target.value >= size) {
            throw std::runtime_error(std::string(owner) + ": invalid target block " + std::to_string(target.value));
        }
    }
}

} // namespace

void validate(const HIRModule &module, IRValidationOptions options) {
    validateBlocks(module.blocks, "HIR", options);
}

void validate(const MIRModule &module, IRValidationOptions options) {
    validateBlocks(module.blocks, "MIR", options);
}



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
    if (module_.blocks.size() >= std::numeric_limits<std::uint32_t>::max()) {
        throw std::runtime_error("HIRBuilder::createBlock: block ID overflow");
    }
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
    if (op.empty() || type == ValueType::Void) {
        throw std::runtime_error("HIRBuilder::emitValue: operation must be non-empty and result type non-void");
    }
    if (nextValue_ == std::numeric_limits<std::uint32_t>::max()) {
        throw std::runtime_error("HIRBuilder::emitValue: value ID overflow");
    }
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
    if (op.empty()) throw std::runtime_error("HIRBuilder::emit: empty operation");
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
    checkTargets(targets, module_.blocks.size(), "HIRBuilder::terminate");

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
    validate(module_);
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
    if (module_.blocks.size() >= std::numeric_limits<std::uint32_t>::max()) {
        throw std::runtime_error("MIRBuilder::createBlock: block ID overflow");
    }
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
    if (op.empty() || type == ValueType::Void) {
        throw std::runtime_error("MIRBuilder::emitValue: operation must be non-empty and result type non-void");
    }
    if (nextValue_ == std::numeric_limits<std::uint32_t>::max()) {
        throw std::runtime_error("MIRBuilder::emitValue: value ID overflow");
    }
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
    if (op.empty()) throw std::runtime_error("MIRBuilder::emit: empty operation");
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
    checkTargets(targets, module_.blocks.size(), "MIRBuilder::terminate");

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
    validate(module_);
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
    validate(hir);
    MIRBuilder builder{};
    MIRLoweringContext context{};

    // Register all blocks up front: a terminator may branch forward.
    for (std::size_t i = 1; i < hir.blocks.size(); ++i) {
        builder.createBlock(hir.blocks[i].name);
    }
    for (const BasicBlock &block : hir.blocks) {
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
