#pragma once

#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/engine/syntax/Parser.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace novac::assets::essentials::internal {

inline bool maybeConsume(parser::ParserContext &context, const std::string &text) {
    if (context.check(text)) {
        context.advance();
        return true;
    }

    return false;
}

inline void maybeConsumeSemicolon(parser::ParserContext &context, const EssentialsController &controller) {
    maybeConsume(context, controller.options().semicolonToken);
}

inline std::vector<std::string> parseParameterList(parser::ParserContext &context, const EssentialsController &controller) {
    const auto &options{controller.options()};
    std::vector<std::string> parameters{};

    context.consume(options.leftParenToken);

    if (!context.check(options.rightParenToken)) {
        while (true) {
            const token::Token &name{context.consumeKind(token::Kind::Identifier)};
            parameters.push_back(name.text);

            if (!maybeConsume(context, options.commaToken))
                break;
        }
    }

    context.consume(options.rightParenToken);

    return parameters;
}

inline ast::NodeList parseArgumentList(parser::ParserContext &context, const EssentialsController &controller) {
    const auto &options{controller.options()};
    ast::NodeList arguments{};

    context.consume(options.leftParenToken);

    if (!context.check(options.rightParenToken)) {
        while (true) {
            arguments.push_back(context.parse(controller.expressionDomain()));

            if (!maybeConsume(context, options.commaToken))
                break;
        }
    }

    context.consume(options.rightParenToken);

    return arguments;
}

} // namespace novac::assets::essentials::internal
