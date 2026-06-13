#include "novac/assets/language/features/presets/StandardExpressionFeatures.hpp"
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

    language.use(UnaryOperatorSyntaxFeature{expressionDomain});
    language.use(ArithmeticOperatorSyntaxFeature{expressionDomain});
    language.use(ComparisonOperatorSyntaxFeature{expressionDomain});
    language.use(LogicalOperatorSyntaxFeature{expressionDomain});

    language.use(StandardUnaryRuntimeFeature{});
    language.use(NumericArithmeticRuntimeFeature{});
    language.use(NumericComparisonRuntimeFeature{});
    language.use(LogicalRuntimeFeature{});
}

void installStandardStatementFeatures(
    LanguageOptionsController &language,
    const std::string &statementDomain,
    const std::string &expressionDomain) {
    language.use(VariableDeclarationFeature{statementDomain, expressionDomain});
    language.use(ExpressionStatementFeature{statementDomain, expressionDomain});
}

} // namespace novac::language::features
