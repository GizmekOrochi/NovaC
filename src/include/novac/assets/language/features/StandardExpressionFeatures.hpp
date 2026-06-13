#pragma once

#include "ArithmeticOperatorsFeature.hpp"
#include "BinaryExpressionFeature.hpp"
#include "BooleanLiteralFeature.hpp"
#include "ComparisonOperatorsFeature.hpp"
#include "ExpressionStatementFeature.hpp"
#include "FloatingLiteralFeature.hpp"
#include "IdentifierExpressionFeature.hpp"
#include "IntegerLiteralFeature.hpp"
#include "LogicalOperatorsFeature.hpp"
#include "ParenthesizedExpressionFeature.hpp"
#include "StringLiteralFeature.hpp"
#include "UnaryExpressionFeature.hpp"
#include "VariableDeclarationFeature.hpp"

#include <string>

namespace novac::language::features {

void installStandardExpressionFeatures(LanguageOptionsController &language, const std::string &expressionDomain = "expr");

} // namespace novac::language::features
