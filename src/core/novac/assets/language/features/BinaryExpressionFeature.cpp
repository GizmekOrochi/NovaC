#include "novac/assets/language/features/BinaryExpressionFeature.hpp"

#include "novac/assets/language/LanguageOptionsController.hpp"

#include <stdexcept>
#include <utility>

namespace novac::language::features {

BinaryExpressionFeature::BinaryExpressionFeature(std::string expressionDomain, std::string nodeKind)
    : expressionDomain_{std::move(expressionDomain)}, nodeKind_{std::move(nodeKind)} {
    if (expressionDomain_.empty() || nodeKind_.empty()) {
        throw std::runtime_error("BinaryExpressionFeature::BinaryExpressionFeature: domain and node kind cannot be empty");
    }
}

LanguageFeatureInfo BinaryExpressionFeature::info() const {
    return {"language.binary-expression", "0.1.0", "Binary expression AST and runtime dispatcher", {"expression.binary"}, {"expression.atom"}, {}, {}, {}};
}

void BinaryExpressionFeature::install(LanguageOptionsController &language) const {
    const std::string kind{nodeKind_};

    language.node({
        .kind = kind,
        .fields = {
            {.name = "op", .kind = ast::FieldKind::String, .required = true},
            {.name = "left", .kind = ast::FieldKind::Node, .required = true, .allowedNodeTraits = {"expr"}},
            {.name = "right", .kind = ast::FieldKind::Node, .required = true, .allowedNodeTraits = {"expr"}}
        },
        .traits = {"expr"},
        .doc = "Binary expression"
    });

    language.setBinaryNodeKind(kind);
}

} // namespace novac::language::features
