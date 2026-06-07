#pragma once

#include "../ast/Node.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace novac::ir {

struct HIRNode {
    std::string op{};
    std::vector<std::string> operands{};
};

struct MIRNode {
    std::string op{};
    std::vector<std::string> operands{};
};

struct HIRModule {
    std::vector<HIRNode> nodes{};
};

struct MIRModule {
    std::vector<MIRNode> nodes{};
};

class HIRBuilder {
public:
    void emit(std::string op, std::vector<std::string> operands = {});

    HIRModule finish();

private:
    HIRModule module_;
};

class MIRBuilder {
public:
    void emit(std::string op, std::vector<std::string> operands = {});

    MIRModule finish();

private:
    MIRModule module_;
};

class LoweringRegistry {
public:
    using HIRLowerer = std::function<void(const ast::Node &, HIRBuilder &, const LoweringRegistry &)>;
    using MIRLowerer = std::function<void(const HIRNode &, MIRBuilder &, const LoweringRegistry &)>;

    bool hir(std::string nodeKind, HIRLowerer fn);
    bool mir(std::string hirKind, MIRLowerer fn);

    bool hasHIR(const std::string &nodeKind) const;
    bool hasMIR(const std::string &hirKind) const;

    void lowerHIR(const ast::Node &node, HIRBuilder &out) const;
    void lowerMIR(const HIRNode &node, MIRBuilder &out) const;
    void lowerChildren(const ast::Node &node, HIRBuilder &out) const;

private:
    std::unordered_map<std::string, HIRLowerer> hir_;
    std::unordered_map<std::string, MIRLowerer> mir_;
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