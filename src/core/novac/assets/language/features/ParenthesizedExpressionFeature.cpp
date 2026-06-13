#include "novac/assets/language/features/ParenthesizedExpressionFeature.hpp"

#include "novac/assets/language/LanguageOptionsController.hpp"

#include <stdexcept>
#include <utility>

namespace novac::language::features {

ParenthesizedExpressionFeature::ParenthesizedExpressionFeature(std::string expressionDomain)
    : expressionDomain_{std::move(expressionDomain)} {
    if (expressionDomain_.empty()) {
        throw std::runtime_error("ParenthesizedExpressionFeature::ParenthesizedExpressionFeature: domain cannot be empty");
    }
}

LanguageFeatureInfo ParenthesizedExpressionFeature::info() const {
    return {"language.parenthesized-expression", "0.1.0", "Parenthesized expression parsing", {"expression.grouping"}, {"expression.atom"}, {}, {}, {}};
}

void ParenthesizedExpressionFeature::install(LanguageOptionsController &language) const {
    const std::string domain{expressionDomain_};

    language.symbol("(");
    language.symbol(")");
    language.prefix(domain, "(", [domain](parser::ParserContext &context) {
        context.consume("(");
        ast::NodePtr expression{context.parse(domain)};
        context.consume(")");
        return expression;
    });
}

} // namespace novac::language::features
