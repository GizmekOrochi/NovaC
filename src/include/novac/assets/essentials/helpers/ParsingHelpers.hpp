#pragma once

#include "novac/engine/syntax/Parser.hpp"

#include <string>
#include <vector>

namespace novac::assets::essentials::helpers {

std::string consumeIdentifier(parser::ParserContext &context, const char *owner);

std::vector<std::string> parseIdentifierList(
    parser::ParserContext &context,
    const std::string &leftParen,
    const std::string &rightParen,
    const std::string &comma
);

ast::NodeList parseExpressionList(
    parser::ParserContext &context,
    const std::string &expressionDomain,
    const std::string &leftParen,
    const std::string &rightParen,
    const std::string &comma
);

} // namespace novac::assets::essentials::helpers
