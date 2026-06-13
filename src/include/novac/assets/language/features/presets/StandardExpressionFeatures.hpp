#pragma once

#include "novac/assets/language/features/operators/ArithmeticOperatorSyntaxFeature.hpp"
#include "novac/assets/language/features/expressions/BinaryExpressionFeature.hpp"
#include "novac/assets/language/features/literals/BooleanLiteralFeature.hpp"
#include "novac/assets/language/features/operators/ComparisonOperatorSyntaxFeature.hpp"
#include "novac/assets/language/features/statements/ExpressionStatementFeature.hpp"
#include "novac/assets/language/features/literals/FloatingLiteralFeature.hpp"
#include "novac/assets/language/features/expressions/IdentifierExpressionFeature.hpp"
#include "novac/assets/language/features/literals/IntegerLiteralFeature.hpp"
#include "novac/assets/language/features/operators/LogicalOperatorSyntaxFeature.hpp"
#include "novac/assets/language/features/runtime/LogicalRuntimeFeature.hpp"
#include "novac/assets/language/features/runtime/NumericArithmeticRuntimeFeature.hpp"
#include "novac/assets/language/features/runtime/NumericComparisonRuntimeFeature.hpp"
#include "novac/assets/language/features/expressions/ParenthesizedExpressionFeature.hpp"
#include "novac/assets/language/features/runtime/StandardUnaryRuntimeFeature.hpp"
#include "novac/assets/language/features/literals/StringLiteralFeature.hpp"
#include "novac/assets/language/features/expressions/UnaryExpressionFeature.hpp"
#include "novac/assets/language/features/operators/UnaryOperatorSyntaxFeature.hpp"
#include "novac/assets/language/features/statements/VariableDeclarationFeature.hpp"

#include <string>

namespace novac::language {

class LanguageOptionsController;

} // namespace novac::language

namespace novac::language::features {

void installStandardExpressionFeatures(
    LanguageOptionsController &language,
    const std::string &expressionDomain = "expr");

void installStandardStatementFeatures(
    LanguageOptionsController &language,
    const std::string &statementDomain = "stmt",
    const std::string &expressionDomain = "expr");

} // namespace novac::language::features
