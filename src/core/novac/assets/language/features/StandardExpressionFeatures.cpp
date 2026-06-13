#include "novac/assets/language/features/StandardExpressionFeatures.hpp"

#include "novac/assets/language/LanguageOptionsController.hpp"

namespace novac::language::features {

void installStandardExpressionFeatures(LanguageOptionsController &language, const std::string &expressionDomain) {
    language.setStartDomain(expressionDomain);
    language.use(IntegerLiteralFeature{expressionDomain});
    language.use(FloatingLiteralFeature{expressionDomain});
    language.use(StringLiteralFeature{expressionDomain});
    language.use(BooleanLiteralFeature{expressionDomain});
    language.use(IdentifierExpressionFeature{expressionDomain});
    language.use(ParenthesizedExpressionFeature{expressionDomain});
    language.use(BinaryExpressionFeature{expressionDomain});
    language.use(UnaryExpressionFeature{expressionDomain});
    language.use(ArithmeticOperatorsFeature{expressionDomain});
    language.use(ComparisonOperatorsFeature{expressionDomain});
    language.use(LogicalOperatorsFeature{expressionDomain});
}

} // namespace novac::language::features
