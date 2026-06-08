#include "../../include/ir/IR.hpp"

#include <stdexcept>
#include <utility>
#include <variant>

namespace novac::ir {

template<class Map, class Value>
registry::RegisterStatus registerEntry(Map &map, std::string key, Value value, registry::DuplicatePolicy duplicatePolicy, const std::string &owner) { 
    const auto iter{map.find(key)};
    
    if (iter != map.end()) {
        if (duplicatePolicy == registry::DuplicatePolicy::Ignore)
            return registry::RegisterStatus::Ignored;

        if (duplicatePolicy == registry::DuplicatePolicy::Replace) {
            iter->second = std::move(value);
            return registry::RegisterStatus::Replaced;
        }

        throw std::runtime_error(owner + ": duplicate registration '" + key + "'");
    }

    map.emplace(std::move(key), std::move(value));
    return registry::RegisterStatus::Inserted;
}

void HIRBuilder::emit(std::string op, std::vector<std::string> operands) {
    module_.nodes.push_back({std::move(op), std::move(operands)});
}

HIRModule HIRBuilder::finish() {
    return std::move(module_);
}

void MIRBuilder::emit(std::string op, std::vector<std::string> operands) {
    module_.nodes.push_back({std::move(op), std::move(operands)});
}

MIRModule MIRBuilder::finish() {
    return std::move(module_);
}

LoweringRegistry::LoweringRegistry(registry::DuplicatePolicy duplicatePolicy)
    : hir_{}, mir_{}, duplicatePolicy_{duplicatePolicy} {}

registry::RegisterStatus LoweringRegistry::hir(std::string nodeKind, HIRLowerer fn) {
    return registerEntry(hir_, std::move(nodeKind), std::move(fn), duplicatePolicy_, "LoweringRegistry::hir");
}

registry::RegisterStatus LoweringRegistry::mir(std::string hirKind, MIRLowerer fn) {
    return registerEntry(mir_, std::move(hirKind),std::move(fn), duplicatePolicy_, "LoweringRegistry::mir");
}

bool LoweringRegistry::hasHIR(const std::string &nodeKind) const {
    return hir_.find(nodeKind) != hir_.end();
}

bool LoweringRegistry::hasMIR(const std::string &hirKind) const {
    return mir_.find(hirKind) != mir_.end();
}

void LoweringRegistry::lowerHIR(const ast::Node &node, HIRBuilder &out) const {
    const auto iter{hir_.find(node.kind())};

    if (iter == hir_.end())
        throw std::runtime_error("LoweringRegistry::lowerHIR: missing HIR lowerer for AST node kind '" + node.kind() + "'");

    iter->second(node, out, *this);
}

void LoweringRegistry::lowerMIR(const HIRNode &node, MIRBuilder &out) const {
    const auto iter{mir_.find(node.op)};

    if (iter == mir_.end())
        throw std::runtime_error("LoweringRegistry::lowerMIR: missing MIR lowerer for HIR node kind '" + node.op + "'");

    iter->second(node, out, *this);
}

void LoweringRegistry::lowerChildren(const ast::Node &node, HIRBuilder &out) const {
    for (const auto &[name, field] : node.fields()) {
        static_cast<void>(name);

        if (const auto *child{std::get_if<ast::NodePtr>(&field)}; child && *child)
            lowerHIR(**child, out);

        if (const auto *list{std::get_if<ast::NodeList>(&field)}) {
            for (const ast::NodePtr &child : *list)
                if (child)
                    lowerHIR(*child, out);
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

    for (const HIRNode &node : hir.nodes)
        registry_.lowerMIR(node, builder);

    return builder.finish();
}

} // namespace novac::ir