#include "novac/assets/atomic/literals/BooleanLiteralAtomic.hpp"
#include "novac/assets/atomic/AtomicIds.hpp"
#include "novac/assets/AssetTraits.hpp"

#include "novac/assets/atomic/AtomicController.hpp"

#include <stdexcept>
#include <utility>
#include <variant>

namespace novac::assets::atomic::literals {

/**
 * @brief Constructs a `BooleanLiteralAtomic` instance.
 *
 * @param nodeKind Value supplied for `nodeKind`.
 * @param tokens Value supplied for `tokens`.
 */
BooleanLiteralAtomic::BooleanLiteralAtomic(std::string nodeKind, BooleanLiteralTokens tokens)
    : nodeKind_{std::move(nodeKind)}, tokens_{std::move(tokens)} {
    if (nodeKind_.empty())
        throw std::runtime_error("BooleanLiteralAtomic::BooleanLiteralAtomic: node kind cannot be empty");

    if (tokens_.trueToken.empty() || tokens_.falseToken.empty())
        throw std::runtime_error("BooleanLiteralAtomic::BooleanLiteralAtomic: boolean tokens cannot be empty");

    if (tokens_.trueToken == tokens_.falseToken)
        throw std::runtime_error("BooleanLiteralAtomic::BooleanLiteralAtomic: true and false tokens must be distinct");
}

/**
 * @brief Implements the `info` operation.
 *
 * @return Value produced by the operation.
 */
LiteralInfo BooleanLiteralAtomic::info() const {
    return {"core.literal.boolean", "0.1.0", "Boolean literal atomic", nodeKind_, TokenPattern::keywordText(tokens_.trueToken), {"literal.boolean", "expression.atom"}, {}};
}

/**
 * @brief Installs the behavior provided by `install`.
 *
 * @param controller Value supplied for `controller`.
 */
void BooleanLiteralAtomic::install(AtomicController &controller) const {
    controller.registerPattern(TokenPattern::keywordText(tokens_.trueToken));
    controller.registerPattern(TokenPattern::keywordText(tokens_.falseToken));

    controller.engine().node({
        .kind = nodeKind_,
        .fields = {{.name = fields::Value.value, .kind = ast::FieldKind::Bool, .required = true}},
        .traits = {novac::assets::traits::Expression, novac::assets::traits::Literal},
        .doc = "Boolean literal expression"
    });

    const std::string domain{controller.expressionDomain()};
    const std::string kind{nodeKind_};
    const std::string trueToken{tokens_.trueToken};
    auto *const engine{&controller.engine()};

    const auto makeBoolean{[kind, trueToken, engine](parser::ParserContext &context) {
        const token::Token item{context.consumeKind(token::Kind::Keyword)};
        ast::NodePtr node{engine->makeNode(kind)};
        node->set(fields::Value, item.text == trueToken);
        return node;
    }};

    controller.engine().prefix(domain, tokens_.trueToken, makeBoolean);
    controller.engine().prefix(domain, tokens_.falseToken, makeBoolean);

    controller.engine().expression(kind, [](const ast::Node &node, runtime::RuntimeContext &) {
        const ast::Field &field{node.field(fields::Value)};
        return runtime::Value::boolean(std::get<bool>(field));
    });
}

} // namespace novac::assets::atomic::literals
