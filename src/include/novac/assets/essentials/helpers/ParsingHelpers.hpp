#pragma once

#include "novac/engine/syntax/Parser.hpp"

#include <string>
#include <vector>

namespace novac::assets::essentials::helpers {

/**
 * @brief Consumes the current identifier token and returns its text.
 *
 * @param context Active parser context.
 * @param owner Human-readable parser owner used in diagnostics.
 * @return Consumed identifier text.
 * @throws std::runtime_error If the current token is not an identifier.
 */
std::string consumeIdentifier(parser::ParserContext &context, const char *owner);

/**
 * @brief Parses a delimiter-enclosed comma-separated identifier list.
 *
 * Empty lists are accepted. The opening and closing tokens are consumed by
 * the helper.
 *
 * @param context Active parser context.
 * @param leftParen Opening delimiter token.
 * @param rightParen Closing delimiter token.
 * @param comma Separator token.
 * @return Identifier texts in source order.
 */
std::vector<std::string> parseIdentifierList(
    parser::ParserContext &context,
    const std::string &leftParen,
    const std::string &rightParen,
    const std::string &comma
);

/**
 * @brief Parses a delimiter-enclosed comma-separated expression list.
 *
 * Empty lists are accepted. Each element is parsed through the supplied
 * expression domain.
 *
 * @param context Active parser context.
 * @param expressionDomain Domain used to parse list elements.
 * @param leftParen Opening delimiter token.
 * @param rightParen Closing delimiter token.
 * @param comma Separator token.
 * @return Parsed expression nodes in source order.
 */
ast::NodeList parseExpressionList(
    parser::ParserContext &context,
    const std::string &expressionDomain,
    const std::string &leftParen,
    const std::string &rightParen,
    const std::string &comma
);

} // namespace novac::assets::essentials::helpers
