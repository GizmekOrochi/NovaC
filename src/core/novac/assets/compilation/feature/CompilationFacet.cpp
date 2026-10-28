#include "novac/assets/compilation/feature/CompilationFacet.hpp"

#include <algorithm>
#include <functional>
#include <unordered_map>
#include <stdexcept>
#include <utility>

namespace novac::compilation {

std::string CompilationFacetRegistry::key(const std::string &kind, const std::string &operation) {
    // Length prefix: unambiguous even when identifiers contain punctuation.
    return std::to_string(kind.size()) + ":" + kind + operation;
}

void CompilationFacetRegistry::attach(CompilationFacet facet) {
    if (facet.featureId.empty() || facet.nodeKind.empty() || facet.operationId.empty() || !facet.lowerHIR) {
        throw std::runtime_error("CompilationFacetRegistry::attach: owner, node kind, operation and lowerer are required");
    }
    const auto identity = key(facet.nodeKind, facet.operationId);
    if (facets_.contains(identity)) {
        throw std::runtime_error("CompilationFacetRegistry::attach: ambiguous handler for '" + facet.nodeKind +
                                 "' and operation '" + facet.operationId + "'");
    }
    for (const auto &requirement : facet.requiredFeatures) {
        if (requirement.empty() || requirement == facet.featureId) {
            throw std::runtime_error("CompilationFacetRegistry::attach: invalid feature dependency");
        }
    }
    owners_.insert(facet.featureId);
    facets_.emplace(identity, std::move(facet));
}

bool CompilationFacetRegistry::has(const std::string &kind, const std::string &operation) const noexcept {
    return facets_.contains(key(kind, operation));
}

void CompilationFacetRegistry::validate() const {
    std::unordered_map<std::string, std::vector<std::string>> dependencyGraph;
    for (const auto &[_, facet] : facets_) {
        auto &deps = dependencyGraph[facet.featureId];
        for (const auto &required : facet.requiredFeatures) {
            if (!owners_.contains(required)) {
                throw std::runtime_error("CompilationFacetRegistry: feature '" + facet.featureId +
                                         "' requires missing feature '" + required + "'");
            }
            deps.push_back(required);
        }
    }
    std::unordered_map<std::string, int> colors;
    std::function<void(const std::string &)> visit = [&](const std::string &name) {
        if (colors[name] == 1) throw std::runtime_error("CompilationFacetRegistry: cyclic feature dependency at '" + name + "'");
        if (colors[name] == 2) return;
        colors[name] = 1;
        for (const auto &next : dependencyGraph[name]) visit(next);
        colors[name] = 2;
    };
    for (const auto &[owner, _] : dependencyGraph) visit(owner);
}

std::optional<ir::ValueId> CompilationFacetRegistry::lowerNode(
    const ast::Node &node, ir::HIRBuilder &builder, const SemanticResolver &semantics,
    std::vector<const ast::Node *> &active, std::vector<ExpectedFeatureType> &expected) const {
    if (active.size() >= 1024 || std::find(active.begin(), active.end(), &node) != active.end()) {
        throw std::runtime_error("CompilationFacetRegistry: cyclic or excessively deep AST at '" + node.kind() + "'");
    }
    const ResolvedOperation resolved = semantics(node);
    if (resolved.id.empty() || !resolved.type.valid()) {
        throw std::runtime_error("CompilationFacetRegistry: invalid semantic resolution for AST '" + node.kind() + "'");
    }
    auto it = facets_.find(key(node.kind(), resolved.id));
    if (it == facets_.end()) {
        throw std::runtime_error("CompilationFacetRegistry: no compilation feature for AST '" +
                                 node.kind() + "', semantic operation '" + resolved.id + "'");
    }
    if (it->second.resultType && *it->second.resultType != resolved.type) {
        throw std::runtime_error("CompilationFacetRegistry: semantic result type/identity mismatch for '" + resolved.id + "'");
    }
    active.push_back(&node);
    struct PopGuard {
        std::vector<const ast::Node *> &stack;
        ~PopGuard() { stack.pop_back(); }
    } guard{active};
    FeatureLoweringContext context{builder, *this, semantics, resolved, active, expected};
    try {
        auto lowered = it->second.lowerHIR(node, context);
        if (resolved.type.isVoid() && lowered) {
            throw std::runtime_error("void semantic operation returned a SSA value");
        }
        if (resolved.type != ir::ValueType::Unknown && !resolved.type.isVoid() && !lowered) {
            throw std::runtime_error("typed semantic operation discarded its result");
        }
        if (lowered) expected.push_back({*lowered, resolved.type, resolved.id});
        return lowered;
    } catch (const std::exception &error) {
        throw std::runtime_error("compiling feature '" + it->second.featureId + "' operation '" +
                                 resolved.id + "': " + error.what());
    }
}

std::optional<ir::ValueId> FeatureLoweringContext::lower(const ast::Node &node) {
    return facets_.lowerNode(node, builder_, resolver_, active_, expected_);
}

ir::ValueId FeatureLoweringContext::requireValue(const ast::Node &node) {
    const auto value = lower(node);
    if (!value) throw std::runtime_error("expected an expression value from AST '" + node.kind() + "'");
    return *value;
}

ir::HIRModule CompilationFacetRegistry::lower(const ast::Node &root, const SemanticResolver &semantics) const {
    if (!semantics) throw std::runtime_error("CompilationFacetRegistry: semantic resolver is required");
    validate();
    ir::HIRBuilder builder{};
    std::vector<const ast::Node *> active{};
    std::vector<ExpectedFeatureType> expected;
    lowerNode(root, builder, semantics, active, expected);
    auto module = builder.finish();
    ir::validate(module, {.requireTerminators = true});
    std::unordered_map<std::uint32_t, ir::TypeRef> defined;
    for (const auto &block : module.blocks) {
        for (const auto &arg : block.arguments) defined.emplace(arg.value.value, arg.type);
        for (const auto &inst : block.instructions) {
            if (inst.result) defined.emplace(inst.result->value, inst.type);
        }
    }
    for (const auto &item : expected) {
        const auto it = defined.find(item.value.value);
        if (it == defined.end() || (item.type != ir::ValueType::Unknown && it->second != item.type)) {
            throw std::runtime_error("CompilationFacetRegistry: semantic result type/identity mismatch for '" +
                                     item.operation + "'");
        }
    }
    return module;
}

} // namespace novac::compilation
