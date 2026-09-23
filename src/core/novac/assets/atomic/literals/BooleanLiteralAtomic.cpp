#include "novac/assets/atomic/literals/BooleanLiteralAtomic.hpp"

#include "novac/assets/atomic/AtomicController.hpp"

#include <stdexcept>
#include <utility>
#include <variant>

namespace novac::assets::atomic::literals {

BooleanLiteralAtomic::BooleanLiteralAtomic(std::string nodeKind, BooleanLiteralTokens tokens)
    : nodeKind_{std::move(nodeKind)}, tokens_{std::move(tokens)} {
    if (nodeKind_.empty()) {
        throw std::runtime_error("BooleanLiteralAtomic::BooleanLiteralAtomic: node kind cannot be empty");
    }

    if (tokens_.trueToken.empty() || tokens_.falseToken.empty()) {
        throw std::runtime_error("BooleanLiteralAtomic::BooleanLiteralAtomic: boolean tokens cannot be empty");
    }
}

LiteralInfo BooleanLiteralAtomic::info() const {
    return {"core.literal.boolean", "0.1.0", "Boolean literal atomic", nodeKind_, TokenPattern::keywordText(tokens_.trueToken), {"literal.boolean", "expression.atom"}, {}};
}

void BooleanLiteralAtomic::install(AtomicController &controller) const {
    controller.registerPattern(TokenPattern::keywordText(tokens_.trueToken));
    controller.registerPattern(TokenPattern::keywordText(tokens_.falseToken));

    controller.engine().node({
        .kind = nodeKind_,
        .fields = {{.name = "value", .kind = ast::FieldKind::Bool, .required = true}},
        .traits = {"expr", "literal"},
        .doc = "Boolean literal expression"
    });

    const std::string domain{controller.expressionDomain()};
    const std::string kind{nodeKind_};
    const std::string trueToken{tokens_.trueToken};
    auto *const engine{&controller.engine()};

    const auto makeBoolean{[kind, trueToken, engine](parser::ParserContext &context) {
        const token::Token item{context.consumeKind(token::Kind::Keyword)};
        ast::NodePtr node{engine->makeNode(kind)};
        node->set("value", item.text == trueToken);
        return node;
    }};

    controller.engine().prefix(domain, tokens_.trueToken, makeBoolean);
    controller.engine().prefix(domain, tokens_.falseToken, makeBoolean);

    controller.engine().expression(kind, [](const ast::Node &node, runtime::RuntimeContext &) {
        const ast::Field &field{node.field("value")};
        return runtime::Value::boolean(std::get<bool>(field));
    });
}

} // namespace novac::assets::atomic::literals
