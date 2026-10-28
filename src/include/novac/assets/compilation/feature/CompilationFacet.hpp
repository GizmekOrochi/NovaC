#pragma once

#include "novac/engine/syntax/Node.hpp"
#include "novac/engine/transformation/IR.hpp"

#include <cstddef>
#include <memory>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace novac::compilation {

/** A language-owned semantic decision, not a guess based on NodeKind. */
struct ResolvedOperation {
    std::string id;
    ir::TypeRef type{ir::ValueType::Unknown};
};

/** Language-defined semantic resolver; must never implicitly evaluate children. */
using SemanticResolver = std::function<ResolvedOperation(const ast::Node &)>;

class CompilationFacetRegistry;
struct ExpectedFeatureType {
    ir::ValueId value;
    ir::TypeRef type;
    std::string operation;
};

/** Context scoped to ONE compilation. Recursive lowering is explicitly ordered
 * by each feature (important for short-circuiting and side effects). */
class FeatureLoweringContext final {
public:
    ir::HIRBuilder &builder() noexcept { return builder_; }
    const ResolvedOperation &semantic() const noexcept { return resolved_; }
    std::optional<ir::ValueId> lower(const ast::Node &node);
    ir::ValueId requireValue(const ast::Node &node);

private:
    friend class CompilationFacetRegistry;
    FeatureLoweringContext(ir::HIRBuilder &builder, const CompilationFacetRegistry &facets,
                           const SemanticResolver &resolver, const ResolvedOperation &resolved,
                           std::vector<const ast::Node *> &active,
                           std::vector<ExpectedFeatureType> &expected)
        : builder_(builder), facets_(facets), resolver_(resolver), resolved_(resolved), active_(active), expected_(expected) {}

    ir::HIRBuilder &builder_;
    const CompilationFacetRegistry &facets_;
    const SemanticResolver &resolver_;
    const ResolvedOperation &resolved_;
    std::vector<const ast::Node *> &active_;
    std::vector<ExpectedFeatureType> &expected_;
};

using FeatureLowerer = std::function<std::optional<ir::ValueId>(const ast::Node &, FeatureLoweringContext &)>;

/** Feature-scoped, portable contribution. No machine code or ABI assumptions. */
struct CompilationFacet {
    std::string featureId;
    std::string nodeKind;
    std::string operationId;
    FeatureLowerer lowerHIR;
    std::vector<std::string> requiredFeatures{};
    // Optional fixed semantic type contract, independent of backend lowering.
    std::optional<ir::TypeRef> resultType{};
};

class CompilationFacetRegistry final {
public:
    /** Reject duplicates instead of silently overriding another feature. */
    void attach(CompilationFacet facet);
    bool has(const std::string &nodeKind, const std::string &operationId) const noexcept;
    void validate() const;

    /** Creates a fresh HIR module; no state leaks between compilation sessions. */
    ir::HIRModule lower(const ast::Node &root, const SemanticResolver &semantics) const;

private:
    friend class FeatureLoweringContext;
    static std::string key(const std::string &nodeKind, const std::string &operationId);
    std::optional<ir::ValueId> lowerNode(const ast::Node &node, ir::HIRBuilder &builder,
                                        const SemanticResolver &semantics,
                                        std::vector<const ast::Node *> &active,
                                        std::vector<ExpectedFeatureType> &expected) const;
    std::unordered_map<std::string, CompilationFacet> facets_{};
    std::unordered_set<std::string> owners_{};
};

} // namespace novac::compilation
