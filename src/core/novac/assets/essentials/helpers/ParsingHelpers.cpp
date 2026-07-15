#include "novac/assets/essentials/helpers/ParsingHelpers.hpp"

#include <stdexcept>

namespace novac::assets::essentials::helpers {

std::string consumeIdentifier(parser::ParserContext &context, const char *owner) {
    const token::Token &name{context.consumeKind(token::Kind::Identifier)};

    if (name.text.empty())
        throw std::runtime_error(std::string{owner} + ": identifier cannot be empty");

    return name.text;
}

std::vector<std::string> parseIdentifierList(parser::ParserContext &context, const std::string &leftParen, const std::string &rightParen, const std::string &comma) {
    std::vector<std::string> names{};

    context.consume(leftParen);

    if (!context.check(rightParen)) {
        while (true) {
            names.push_back(consumeIdentifier(context, "parseIdentifierList"));

            if (!context.check(comma))
                break;

            context.consume(comma);
        }
    }

    context.consume(rightParen);

    return names;
}

ast::NodeList parseExpressionList(parser::ParserContext &context, const std::string &expressionDomain, const std::string &leftParen, const std::string &rightParen, const std::string &comma) {
    ast::NodeList args{};

    context.consume(leftParen);

    if (!context.check(rightParen)) {
        while (true) {
            args.push_back(context.parse(expressionDomain));

            if (!context.check(comma))
                break;

            context.consume(comma);
        }
    }

    context.consume(rightParen);

    return args;
}

} // namespace novac::assets::essentials::helpers
