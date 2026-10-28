#include "novac/engine/transformation/IR.hpp"

#include <limits>
#include <algorithm>
#include <exception>
#include <stdexcept>
#include <utility>
#include <variant>

namespace novac::ir {

namespace {

struct ValueDefinition {
    std::size_t block{};
    std::size_t instruction{};
    ValueType type{ValueType::Unknown};
    bool parameter{false};
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
        if (b == 0 && !block.arguments.empty()) {
            throw std::runtime_error(where + "entry block requires an external signature; parameters not allowed");
        }
        for (std::size_t i = 0; i < block.arguments.size(); ++i) {
            const BlockArgument &arg = block.arguments[i];
            if (arg.type == ValueType::Void) throw std::runtime_error(where + "void block parameter");
            if (!definitions.emplace(arg.value.value, ValueDefinition{b, 0, arg.type, true}).second) {
                throw std::runtime_error(where + "duplicate value v" + std::to_string(arg.value.value));
            }
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
            if (instruction.result && !definitions.emplace(instruction.result->value, ValueDefinition{b, i, instruction.type, false}).second) {
                throw std::runtime_error(where + "duplicate value v" + std::to_string(instruction.result->value));
            }
        }
        if (!block.terminator) continue;
        const Terminator &term = *block.terminator;
        if (term.op.empty()) throw std::runtime_error(where + "empty terminator operation");
        // These operations have engine-defined CFG semantics; extension-defined
        // terminators retain their own operation-specific successor contracts.
        if (term.op == "branch" && term.operands.size() > 1) {
            throw std::runtime_error(where + "branch accepts a single condition operand");
        }
        if ((term.op == "jump" && term.successors.size() != 1) ||
            (term.op == "branch" && term.successors.size() != 2) ||
            (term.op == "return" && !term.successors.empty())) {
            throw std::runtime_error(where + "invalid successor count for '" + term.op + "'");
        }
        for (std::size_t e = 0; e < term.successors.size(); ++e) {
            const Successor &edge = term.successors[e];
            if (edge.target.value >= blocks.size()) {
                throw std::runtime_error(where + "invalid target block " + std::to_string(edge.target.value));
            }
            const BasicBlock &dest = blocks[edge.target.value];
            if (edge.arguments.size() != dest.arguments.size()) {
                throw std::runtime_error(where + "edge " + std::to_string(e) + " to block " +
                    std::to_string(edge.target.value) + " has wrong argument count");
            }
            successors[b].push_back(edge.target.value);
            predecessors[edge.target.value].push_back(b);
        }
    }

    // Reachability and reverse postorder, computed without recursion so even
    // pathological deeply nested CFGs cannot overflow the call stack.
    // Non-reachable blocks may still define and use their local values only.
    const std::size_t noBlock = blocks.size();
    std::vector<bool> reachable(blocks.size(), false);
    std::vector<std::size_t> postorder{};
    struct Frame { std::size_t block, child; };
    std::vector<Frame> work{{0, 0}};
    reachable[0] = true;
    while (!work.empty()) {
        Frame &frame = work.back();
        if (frame.child == successors[frame.block].size()) {
            postorder.push_back(frame.block);
            work.pop_back();
            continue;
        }
        const std::size_t next = successors[frame.block][frame.child++];
        if (!reachable[next]) {
            reachable[next] = true;
            work.push_back({next, 0});
        }
    }
    std::vector<std::size_t> rpo(postorder.rbegin(), postorder.rend());
    std::vector<std::size_t> rank(blocks.size(), noBlock);
    for (std::size_t i = 0; i < rpo.size(); ++i) rank[rpo[i]] = i;

    // Immediate dominators (Cooper et al.) need O(B+E) storage rather than
    // the previous dense O(B^2) boolean table. Cyclic CFGs are supported.
    std::vector<std::size_t> idom(blocks.size(), noBlock);
    idom[0] = 0;
    const auto intersect = [&](std::size_t left, std::size_t right) {
        while (left != right) {
            while (rank[left] > rank[right]) left = idom[left];
            while (rank[right] > rank[left]) right = idom[right];
        }
        return left;
    };
    bool changed = true;
    while (changed) {
        changed = false;
        for (std::size_t index = 1; index < rpo.size(); ++index) {
            const std::size_t block = rpo[index];
            std::size_t newIdom = noBlock;
            for (std::size_t pred : predecessors[block]) {
                if (idom[pred] == noBlock) continue;
                newIdom = newIdom == noBlock ? pred : intersect(pred, newIdom);
            }
            if (newIdom == noBlock) {
                throw std::runtime_error(prefix + "internal CFG dominator analysis failure");
            }
            if (idom[block] != newIdom) {
                idom[block] = newIdom;
                changed = true;
            }
        }
    }

    // Dominator-tree preorder intervals allow O(1) dominance queries.
    std::vector<std::vector<std::size_t>> domChildren(blocks.size());
    for (std::size_t i = 1; i < rpo.size(); ++i) {
        domChildren[idom[rpo[i]]].push_back(rpo[i]);
    }
    std::vector<std::size_t> enter(blocks.size(), 0), leave(blocks.size(), 0);
    std::size_t tick = 0;
    std::vector<Frame> domWalk{{0, 0}};
    enter[0] = ++tick;
    while (!domWalk.empty()) {
        Frame &frame = domWalk.back();
        if (frame.child == domChildren[frame.block].size()) {
            leave[frame.block] = ++tick;
            domWalk.pop_back();
        } else {
            const std::size_t next = domChildren[frame.block][frame.child++];
            enter[next] = ++tick;
            domWalk.push_back({next, 0});
        }
    }
    const auto dominates = [&](std::size_t definition, std::size_t use) {
        return reachable[definition] && reachable[use] &&
               enter[definition] <= enter[use] && leave[use] <= leave[definition];
    };

    const auto checkOperands = [&](const std::vector<Operand> &operands, std::size_t block, std::size_t instruction, const std::string &where) {
        for (const Operand &operand : operands) {
            if (!operand.isValue()) continue; // Literal/symbol types are language-defined.
            const std::uint32_t v = operand.asValue().value;
            const auto it = definitions.find(v);
            if (it == definitions.end()) throw std::runtime_error(where + "undefined value v" + std::to_string(v));
            const ValueDefinition def = it->second;
            if (def.block == block) {
                if (!def.parameter && def.instruction >= instruction) {
                    throw std::runtime_error(where + "use of v" + std::to_string(v) + " before its definition");
                }
            } else if (!reachable[block] || !dominates(def.block, block)) {
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
            // Legacy branch without a condition is retained for old CFG tests.
            // Typed conditions are checked; Unknown and symbolic conditions
            // need a language-level runtime boolean contract.
            if (block.terminator->op == "branch" && !block.terminator->operands.empty()) {
                const Operand &cond = block.terminator->operands.front();
                if (cond.isValue()) {
                    const ValueType type = definitions.at(cond.asValue().value).type;
                    if (type != ValueType::Bool && type != ValueType::Unknown)
                        throw std::runtime_error(prefix + "block " + std::to_string(b) + ": branch condition must be bool or unknown");
                } else if (const auto *literal = std::get_if<Literal>(&cond.data())) {
                    if (!std::holds_alternative<bool>(literal->data()))
                        throw std::runtime_error(prefix + "block " + std::to_string(b) + ": branch condition literal must be bool");
                }
            }
            for (std::size_t e = 0; e < block.terminator->successors.size(); ++e) {
                const Successor &edge = block.terminator->successors[e];
                const auto &formals = blocks[edge.target.value].arguments;
                for (std::size_t i = 0; i < edge.arguments.size(); ++i) {
                    const Operand &input = edge.arguments[i];
                    const std::string where = prefix + "block " + std::to_string(b) +
                        " edge " + std::to_string(e) + " argument " + std::to_string(i) + ": ";
                    if (!input.isValue()) throw std::runtime_error(where + "edge arguments must be SSA values");
                    checkOperands({input}, b, block.instructions.size(), where);
                    const auto def = definitions.find(input.asValue().value);
                    if (def->second.type != formals[i].type) {
                        throw std::runtime_error(where + "argument type does not match destination parameter");
                    }
                }
            }
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

void checkSuccessors(const std::vector<Successor> &successors, std::size_t size, const char *owner) {
    for (const auto &edge : successors) {
        if (edge.target.value >= size) {
            throw std::runtime_error(std::string(owner) + ": invalid target block " + std::to_string(edge.target.value));
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

BlockId HIRBuilder::createBlock(std::string name, const std::vector<ValueType> &types) {
    // Validate before adding the block so a bad parameter list doesn't mutate the builder.
    for (const auto type : types) if (type == ValueType::Void)
        throw std::runtime_error("HIRBuilder::createBlock: void block parameter");
    if (types.size() > std::numeric_limits<std::uint32_t>::max() - nextValue_)
        throw std::runtime_error("HIRBuilder::createBlock: value ID overflow");
    BlockId id = createBlock(std::move(name));
    for (ValueType type : types) addBlockArgument(id, type);
    return id;
}

ValueId HIRBuilder::addBlockArgument(BlockId block, ValueType type) {
    if (block.value >= module_.blocks.size() || block.value == 0)
        throw std::runtime_error("HIRBuilder::addBlockArgument: invalid block or entry signature");
    if (type == ValueType::Void || nextValue_ == std::numeric_limits<std::uint32_t>::max())
        throw std::runtime_error("HIRBuilder::addBlockArgument: void type or value ID overflow");
    if (module_.blocks[block.value].terminator || !module_.blocks[block.value].instructions.empty())
        throw std::runtime_error("HIRBuilder::addBlockArgument: cannot append to populated block");
    for (const auto &other : module_.blocks) {
        if (!other.terminator) continue;
        for (const auto &edge : other.terminator->successors) {
            if (edge.target.value == block.value)
                throw std::runtime_error("HIRBuilder::addBlockArgument: block already has incoming edges");
        }
    }
    ValueId value{nextValue_++};
    module_.blocks[block.value].arguments.push_back(BlockArgument{value,type});
    return value;
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
    checkTargets(targets, module_.blocks.size(), "HIRBuilder::terminate");
    std::vector<Successor> edges;
    edges.reserve(targets.size());
    for (BlockId target : targets) edges.emplace_back(target);
    terminateWithSuccessors(std::move(op), std::move(operands), std::move(edges));
}

void HIRBuilder::terminateWithSuccessors(std::string op, std::vector<Operand> operands,
                                             std::vector<Successor> successors) {
    if (op.empty()) throw std::runtime_error("HIRBuilder::terminate: empty operation");
    if (currentBlock().terminator) throw std::runtime_error("HIRBuilder::terminate: block already terminated");
    checkSuccessors(successors, module_.blocks.size(), "HIRBuilder::terminate");
    currentBlock().terminator = Terminator{std::move(op), std::move(operands), std::move(successors)};
}

void HIRBuilder::terminateWithSuccessors(const ids::Operation &op, std::vector<Operand> operands,
                                             std::vector<Successor> successors) {
    terminateWithSuccessors(op.value, std::move(operands), std::move(successors));
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

BlockId MIRBuilder::createBlock(std::string name, const std::vector<ValueType> &types) {
    // Validate before adding the block so a bad parameter list doesn't mutate the builder.
    for (const auto type : types) if (type == ValueType::Void)
        throw std::runtime_error("MIRBuilder::createBlock: void block parameter");
    if (types.size() > std::numeric_limits<std::uint32_t>::max() - nextValue_)
        throw std::runtime_error("MIRBuilder::createBlock: value ID overflow");
    BlockId id = createBlock(std::move(name));
    for (ValueType type : types) addBlockArgument(id, type);
    return id;
}

ValueId MIRBuilder::addBlockArgument(BlockId block, ValueType type) {
    if (block.value >= module_.blocks.size() || block.value == 0)
        throw std::runtime_error("MIRBuilder::addBlockArgument: invalid block or entry signature");
    if (type == ValueType::Void || nextValue_ == std::numeric_limits<std::uint32_t>::max())
        throw std::runtime_error("MIRBuilder::addBlockArgument: void type or value ID overflow");
    if (module_.blocks[block.value].terminator || !module_.blocks[block.value].instructions.empty())
        throw std::runtime_error("MIRBuilder::addBlockArgument: cannot append to populated block");
    for (const auto &other : module_.blocks) {
        if (!other.terminator) continue;
        for (const auto &edge : other.terminator->successors) {
            if (edge.target.value == block.value)
                throw std::runtime_error("MIRBuilder::addBlockArgument: block already has incoming edges");
        }
    }
    ValueId value{nextValue_++};
    module_.blocks[block.value].arguments.push_back(BlockArgument{value,type});
    valueTypes_.emplace(value.value, type);
    return value;
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

    ++insertionChanges_;
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
    valueTypes_.emplace(result.value, type);

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
    checkTargets(targets, module_.blocks.size(), "MIRBuilder::terminate");
    std::vector<Successor> edges;
    edges.reserve(targets.size());
    for (BlockId target : targets) edges.emplace_back(target);
    terminateWithSuccessors(std::move(op), std::move(operands), std::move(edges));
}

void MIRBuilder::terminateWithSuccessors(std::string op, std::vector<Operand> operands,
                                             std::vector<Successor> successors) {
    if (op.empty()) throw std::runtime_error("MIRBuilder::terminate: empty operation");
    if (currentBlock().terminator) throw std::runtime_error("MIRBuilder::terminate: block already terminated");
    checkSuccessors(successors, module_.blocks.size(), "MIRBuilder::terminate");
    currentBlock().terminator = Terminator{std::move(op), std::move(operands), std::move(successors)};
}

void MIRBuilder::terminateWithSuccessors(const ids::Operation &op, std::vector<Operand> operands,
                                             std::vector<Successor> successors) {
    terminateWithSuccessors(op.value, std::move(operands), std::move(successors));
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
    if (!values_.emplace(hirValue.value, mirValue).second) {
        throw std::runtime_error("MIRLoweringContext::bind: duplicate HIR binding v" + std::to_string(hirValue.value));
    }
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
MIRLoweringResult LoweringRegistry::lowerMIR(const Instruction &instruction, MIRBuilder &out, MIRLoweringContext &context) const {
    const auto iter{mir_.find(instruction.op)};
    if (iter == mir_.end()) {
        throw std::runtime_error("LoweringRegistry::lowerMIR: missing MIR lowerer for HIR instruction '" + instruction.op + "'");
    }

    // The callback receives only MIR-mapped operands as its normal input.
    // The original source remains accessible for metadata and diagnostics.
    MIRLoweringInput input{instruction, context.remapOperands(instruction.operands)};
    const auto &before{out.inspect()};
    const std::size_t blockCount = before.blocks.size();
    const BlockId insertion = out.currentBlockId();
    const std::size_t insertionChanges = out.insertionChanges();
    std::vector<std::size_t> counts{};
    std::vector<bool> terminated{};
    counts.reserve(blockCount);
    terminated.reserve(blockCount);
    for (const auto &block : before.blocks) {
        counts.push_back(block.instructions.size());
        terminated.push_back(block.terminator.has_value());
    }

    MIRLoweringResult lowered = iter->second(input, out, context, *this);
    const auto &after{out.inspect()};
    if (after.blocks.size() != blockCount || out.currentBlockId().value != insertion.value ||
        out.insertionChanges() != insertionChanges) {
        throw std::runtime_error("LoweringRegistry::lowerMIR: lowerer changed the CFG or insertion block");
    }
    for (std::size_t b = 0; b < blockCount; ++b) {
        if (after.blocks[b].instructions.size() < counts[b] ||
            (b != insertion.value && after.blocks[b].instructions.size() != counts[b]) ||
            after.blocks[b].terminator.has_value() != terminated[b]) {
            throw std::runtime_error("LoweringRegistry::lowerMIR: lowerer modified another block or its terminator");
        }
    }
    if (lowered.value && lowered.explicitlyElided) {
        throw std::runtime_error("LoweringRegistry::lowerMIR: conflicting result and elision");
    }
    if (!instruction.result && (lowered.value || lowered.explicitlyElided || lowered.declaredConversion)) {
        throw std::runtime_error("LoweringRegistry::lowerMIR: void HIR instruction returned a result or elision");
    }
    if (instruction.result && !lowered.value && !lowered.explicitlyElided) {
        throw std::runtime_error("LoweringRegistry::lowerMIR: HIR result silently discarded");
    }
    if (lowered.declaredConversion && !lowered.value) {
        throw std::runtime_error("LoweringRegistry::lowerMIR: conversion has no result");
    }
    if (lowered.value) {
        const auto mapped = out.valueType(*lowered.value);
        const bool defined = mapped.has_value();
        const ValueType mappedType = mapped.value_or(ValueType::Unknown);
        if (!defined) {
            throw std::runtime_error("LoweringRegistry::lowerMIR: lowerer returned undefined MIR value v" +
                                     std::to_string(lowered.value->value));
        }
        if (!lowered.declaredConversion && instruction.type != mappedType) {
            throw std::runtime_error("LoweringRegistry::lowerMIR: value type changed from " +
                                     valueTypeName(instruction.type) + " to " + valueTypeName(mappedType) +
                                     " without declared conversion");
        }
        context.bind(*instruction.result, *lowered.value);
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
MIRModule HIRLoweringPass::lower(const HIRModule &hir, HIRLoweringOptions options) const {
    validate(hir, options.validation);
    MIRBuilder builder{};
    MIRLoweringContext context{};
    const std::size_t blockCount = hir.blocks.size();

    // Preserve stable block IDs, including forward targets. Callback code is
    // not allowed to change the CFG inside this structure-preserving pass.
    for (std::size_t i = 1; i < blockCount; ++i) {
        builder.createBlock(hir.blocks[i].name);
    }

    // Predeclare every MIR block parameter before DFS, including cyclic backedges.
    // All block parameters share the very same builder allocator as instruction results.
    for (std::size_t b = 1; b < blockCount; ++b) {
        for (const auto &arg : hir.blocks[b].arguments) {
            context.bind(arg.value, builder.addBlockArgument(BlockId{static_cast<std::uint32_t>(b)}, arg.type));
        }
    }

    // DFS preorder visits a dominator before every dominated reachable block,
    // irrespective of the physical block order. Iterative DFS avoids stack
    // overflow for deeply nested control-flow graphs.
    std::vector<std::size_t> order{};
    std::vector<bool> seen(blockCount, false);
    std::vector<std::size_t> stack{0};
    while (!stack.empty()) {
        const std::size_t b = stack.back();
        stack.pop_back();
        if (seen[b]) continue;
        seen[b] = true;
        order.push_back(b);
        if (hir.blocks[b].terminator) {
            const auto &targets = hir.blocks[b].terminator->successors;
            for (auto it = targets.rbegin(); it != targets.rend(); ++it) {
                if (!seen[it->target.value]) stack.push_back(it->target.value);
            }
        }
    }
    // Unreachable blocks are permitted by the structural IR validator, but
    // their use-def references must be local; process them independently.
    for (std::size_t b = 0; b < blockCount; ++b) {
        if (!seen[b]) order.push_back(b);
    }

    struct Emission {
        std::size_t block;
        std::size_t begin;
        std::size_t end;
        metadata::MetadataStore metadata;
    };
    std::vector<Emission> emissions{};
    struct BoundUse { std::size_t block, insertion; ValueId mirValue; };
    std::vector<BoundUse> boundUses{};

    for (std::size_t b : order) {
        const BasicBlock &block = hir.blocks[b];
        builder.setCurrentBlock(block.id);
        for (std::size_t i = 0; i < block.instructions.size(); ++i) {
            const Instruction &instruction = block.instructions[i];
            const std::size_t begin = builder.inspect().blocks[b].instructions.size();
            try {
                MIRLoweringResult mapped = registry_.lowerMIR(instruction, builder, context);
                if (mapped.value) {
                    boundUses.push_back({b, builder.inspect().blocks[b].instructions.size(), *mapped.value});
                }
                if (builder.inspect().blocks[b].instructions.size() > begin && options.propagateMetadata) {
                    emissions.push_back({b, begin, builder.inspect().blocks[b].instructions.size(), instruction.metadata});
                }
            } catch (const std::exception &error) {
                throw std::runtime_error("HIR->MIR lowering: block " + std::to_string(b) +
                    " ('" + block.name + "') instruction " + std::to_string(i) + " '" + instruction.op +
                    "'" + (instruction.result ? " HIR v" + std::to_string(instruction.result->value) : "") +
                    ": " + error.what());
            }
        }
        if (block.terminator) {
            try {
                std::vector<Successor> remapped{};
                remapped.reserve(block.terminator->successors.size());
                for (const Successor &edge : block.terminator->successors) {
                    remapped.emplace_back(edge.target, context.remapOperands(edge.arguments));
                    if (options.propagateMetadata) remapped.back().metadata = edge.metadata;
                }
                builder.terminateWithSuccessors(block.terminator->op,
                    context.remapOperands(block.terminator->operands), std::move(remapped));
            } catch (const std::exception &error) {
                throw std::runtime_error("HIR->MIR lowering: block " + std::to_string(b) +
                                         " terminator '" + block.terminator->op + "': " + error.what());
            }
        }
    }
    MIRModule result{builder.finish()};
    result.blocks[0].name = hir.blocks[0].name;
    if (options.propagateMetadata) {
        result.metadata = hir.metadata;
        for (std::size_t b = 0; b < blockCount; ++b) {
            result.blocks[b].metadata = hir.blocks[b].metadata;
            for (std::size_t i = 0; i < hir.blocks[b].arguments.size(); ++i) {
                result.blocks[b].arguments[i].metadata = hir.blocks[b].arguments[i].metadata;
            }
            if (hir.blocks[b].terminator) {
                result.blocks[b].terminator->metadata = hir.blocks[b].terminator->metadata;
            }
        }
        // Every MIR instruction emitted by one callback inherits its source
        // metadata. Callbacks must attach more specific metadata themselves in
        // a future richer IR rewriting API; this pass preserves provenance.
        for (const auto &e : emissions) {
            for (std::size_t i = e.begin; i < e.end; ++i) {
                result.blocks[e.block].instructions[i].metadata = e.metadata;
            }
        }
    }
    validate(result, options.validation);

    // Validate aliases even when they are never used by the source HIR. By
    // inserting temporary uses at the precise bind sites into a throwaway MIR
    // copy, the normal validator checks both same-block ordering and inter-
    // block dominance. No validation instructions escape into the result.
    if (!boundUses.empty()) {
        MIRModule bindingsChecked{result};
        std::vector<std::vector<BoundUse>> usesByBlock(blockCount);
        for (const BoundUse &binding : boundUses) usesByBlock[binding.block].push_back(binding);
        for (std::size_t b = 0; b < blockCount; ++b) {
            const auto &uses = usesByBlock[b];
            if (uses.empty()) continue;
            const auto &original = result.blocks[b].instructions;
            std::vector<Instruction> checked{};
            checked.reserve(original.size() + uses.size());
            std::size_t cursor = 0;
            for (const BoundUse &binding : uses) {
                while (cursor < binding.insertion) checked.push_back(original[cursor++]);
                checked.push_back({"novac.ir.check.binding", std::nullopt,
                                   {Operand::fromValue(binding.mirValue)}, ValueType::Void});
            }
            while (cursor < original.size()) checked.push_back(original[cursor++]);
            bindingsChecked.blocks[b].instructions = std::move(checked);
        }
        try {
            validate(bindingsChecked, options.validation);
        } catch (const std::exception &error) {
            throw std::runtime_error(std::string("HIR->MIR lowering: invalid mapped result: ") + error.what());
        }
    }
    return result;
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
