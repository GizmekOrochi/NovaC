#include "novac/assets/language/features/expressions/UnaryExpressionFeature.hpp"
#include "novac/assets/language/LanguageOptionsController.hpp"

#include <stdexcept>
#include <utility>

namespace novac::language::features {

UnaryExpressionFeature::UnaryExpressionFeature(std::string expressionDomain, std::string nodeKind)
    : expressionDomain_{std::move(expressionDomain)}, nodeKind_{std::move(nodeKind)} {
    if (expressionDomain_.empty() || nodeKind_.empty()) {
        throw std::runtime_error("UnaryExpressionFeature::UnaryExpressionFeature: domain and node kind cannot be empty");
    }
}

LanguageFeatureInfo UnaryExpressionFeature::info() const {
    return {"language.expression.unary", "0.2.0", "Unary expression AST", {"expression.unary"}, {"expression.atom"}, {}, {}, {}};
}

void UnaryExpressionFeature::install(LanguageOptionsController &language) const {
    const std::string kind{nodeKind_};

    language.node({
        .kind = kind,
        .fields = {
            {.name = "op", .kind = ast::FieldKind::String, .required = true},
            {.name = "expr", .kind = ast::FieldKind::Node, .required = true, .allowedNodeTraits = {"expr"}}
        },
        .traits = {"expr"},
        .doc = "Unary expression"
    });
}

} // namespace novac::language::features
