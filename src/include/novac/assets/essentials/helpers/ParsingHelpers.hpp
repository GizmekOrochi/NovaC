#pragma once

#include "novac/engine/syntax/Parser.hpp"

#include <string>
#include <vector>

namespace novac::assets::essentials::helpers {

/**
 * @brief Consumes the current identifier token and returns its text.
 *
 * The helper requires the current token to be an identifier and also rejects
 * identifiers whose text is empty.
 *
 * @param context Active parser context.
 * @param owner Human-readable parser owner included in validation errors.
 * @return Consumed identifier text.
 *
 * @throws std::runtime_error If the current token is not an identifier or the
 * consumed identifier text is empty.
 */
std::string consumeIdentifier(parser::ParserContext &context, const char *owner);

/**
 * @brief Parses a delimiter-enclosed comma-separated identifier list.
 *
 * The opening delimiter is consumed first. Identifiers are then read in source
 * order until the closing delimiter is reached, with the configured comma token
 * consumed between entries.
 *
 * Empty lists are accepted when the closing delimiter immediately follows the
 * opening delimiter.
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
 * The opening delimiter is consumed first. Each element is parsed through the
 * supplied expression domain and appended to the result in source order.
 *
 * Parsing stops when no comma follows the current expression, after which the
 * closing delimiter is consumed. Empty lists are accepted.
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
