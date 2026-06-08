#pragma once

#include "../ast/Node.hpp"
#include "../ids/Ids.hpp"
#include "../registry/Registry.hpp"
#include "../types/TypeSystem.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace novac::ir {

struct ValueId {
    std::uint32_t value{};
};

struct BlockId {
    std::uint32_t value{};
};

class Literal {
public:
    using Data = std::variant<std::monostate, int, double, bool, std::string>;

    Literal();
    explicit Literal(Data data);

    static Literal integer(int value);
    static Literal floating(double value);
    static Literal boolean(bool value);
    static Literal string(std::string value);

    std::string toString() const;

private:
    Data data_;
};

class Operand {
public:
    using Data = std::variant<ValueId, Literal, std::string>;

    explicit Operand(Data data);

    static Operand fromValue(ValueId id);
    static Operand fromLiteral(Literal literal);
    static Operand fromLiteral(int value);
    static Operand fromLiteral(double value);
    static Operand fromLiteral(bool value);
    static Operand fromLiteral(std::string value);
    static Operand fromSymbol(std::string name);

    std::string toString() const;

private:
    Data data_;
};

struct Instruction {
    std::string op{};
    std::optional<ValueId> result{};
    std::vector<Operand> operands{};
    types::TypeRef type{};
};

struct BasicBlock {
    BlockId id{};
    std::string name{};
    std::vector<Instruction> instructions{};
};

struct HIRModule {
    std::vector<BasicBlock> blocks{};
};

struct MIRModule {
    std::vector<BasicBlock> blocks{};
};

class HIRBuilder {
public:
    HIRBuilder();

    BlockId createBlock(std::string name);
    void setCurrentBlock(BlockId block);

    ValueId emitValue(
        std::string op,
        std::vector<Operand> operands = {},
        types::TypeRef type = nullptr);

    ValueId emitValue(
        const ids::Operation &op,
        std::vector<Operand> operands = {},
        types::TypeRef type = nullptr);

    void emit(
        std::string op,
        std::vector<Operand> operands = {});

    void emit(
        const ids::Operation &op,
        std::vector<Operand> operands = {});

    HIRModule finish();

private:
    BasicBlock &currentBlock();

    HIRModule module_;
    BlockId currentBlock_;
    std::uint32_t nextValue_;
};

class MIRBuilder {
public:
    MIRBuilder();

    BlockId createBlock(std::string name);
    void setCurrentBlock(BlockId block);

    ValueId emitValue(
        std::string op,
        std::vector<Operand> operands = {},
        types::TypeRef type = nullptr);

    ValueId emitValue(
        const ids::Operation &op,
        std::vector<Operand> operands = {},
        types::TypeRef type = nullptr);

    void emit(
        std::string op,
        std::vector<Operand> operands = {});

    void emit(
        const ids::Operation &op,
        std::vector<Operand> operands = {});

    MIRModule finish();

private:
    BasicBlock &currentBlock();

    MIRModule module_;
    BlockId currentBlock_;
    std::uint32_t nextValue_;
};

class LoweringRegistry {
public:
    using HIRLowerer = std::function<void(const ast::Node &, HIRBuilder &, const LoweringRegistry &)>;
    using MIRLowerer = std::function<void(const Instruction &, MIRBuilder &, const LoweringRegistry &)>;

    explicit LoweringRegistry(
        registry::DuplicatePolicy duplicatePolicy = registry::DuplicatePolicy::Error);

    registry::RegisterStatus hir(std::string nodeKind, HIRLowerer fn);
    registry::RegisterStatus hir(const ids::NodeKind &nodeKind, HIRLowerer fn);

    registry::RegisterStatus mir(std::string hirKind, MIRLowerer fn);
    registry::RegisterStatus mir(const ids::Operation &hirKind, MIRLowerer fn);

    bool hasHIR(const std::string &nodeKind) const;
    bool hasHIR(const ids::NodeKind &nodeKind) const;

    bool hasMIR(const std::string &hirKind) const;
    bool hasMIR(const ids::Operation &hirKind) const;

    void lowerHIR(const ast::Node &node, HIRBuilder &out) const;
    void lowerMIR(const Instruction &instruction, MIRBuilder &out) const;
    void lowerChildren(const ast::Node &node, HIRBuilder &out) const;

private:
    std::unordered_map<std::string, HIRLowerer> hir_;
    std::unordered_map<std::string, MIRLowerer> mir_;
    registry::DuplicatePolicy duplicatePolicy_;
};

class ASTLoweringPass {
public:
    explicit ASTLoweringPass(const LoweringRegistry &registry);

    HIRModule lower(const ast::Node &root) const;

private:
    const LoweringRegistry &registry_;
};

class HIRLoweringPass {
public:
    explicit HIRLoweringPass(const LoweringRegistry &registry);

    MIRModule lower(const HIRModule &hir) const;

private:
    const LoweringRegistry &registry_;
};

} // namespace novac::ir