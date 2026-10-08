#include "novac/assets/atomic/literals/IntegerLiteralAtomic.hpp"
#include "novac/assets/atomic/AtomicIds.hpp"
#include "novac/assets/AssetTraits.hpp"

#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/assets/atomic/literals/LiteralParsing.hpp"

#include <stdexcept>
#include <utility>

namespace novac::assets::atomic::literals {


/**
 * @brief Constructs a `IntegerLiteralAtomic` instance.
 *
 * @param nodeKind Value supplied for `nodeKind`.
 * @param pattern Value supplied for `pattern`.
 */
IntegerLiteralAtomic::IntegerLiteralAtomic(std::string nodeKind, TokenPattern pattern)
    : nodeKind_{std::move(nodeKind)}, pattern_{std::move(pattern)} {
    if (nodeKind_.empty())
        throw std::runtime_error("IntegerLiteralAtomic::IntegerLiteralAtomic: node kind cannot be empty");
}

/**
 * @brief Implements the `info` operation.
 *
 * @return Value produced by the operation.
 */
LiteralInfo IntegerLiteralAtomic::info() const {
    return {"core.literal.integer", "0.1.0", "Integer literal atomic", nodeKind_, pattern_, {"literal.integer", "expression.atom"}, {}};
}

/**
 * @brief Installs the behavior provided by `install`.
 *
 * @param controller Value supplied for `controller`.
 */
void IntegerLiteralAtomic::install(AtomicController& controller) const {
    controller.registerPattern(pattern_);

    controller.engine().node({
        .kind = nodeKind_,
        .fields = {
            {
                .name = fields::Value.value,
                .kind = ast::FieldKind::Int,
                .required = true
            }
        },
        .traits = {novac::assets::traits::Expression, novac::assets::traits::Literal},
        .doc = "Integer literal expression"
    });

    const std::string domain{controller.expressionDomain()};
    const std::string kind{nodeKind_};
    const auto preparedPattern{detail::prepareSuffixPattern(pattern_, "IntegerLiteralAtomic::install")};
    const TokenPattern &pattern{preparedPattern.pattern};
    const std::string key{pattern.tokenKey.empty() ? pattern.token : pattern.tokenKey};
    auto *const engine{&controller.engine()};

    controller.engine().prefix(domain, key, [kind, preparedPattern, engine](parser::ParserContext& context) {
        const token::Token item{context.consumeKind(token::Kind::Integer)};
        detail::validateSuffix(preparedPattern, item, "IntegerLiteralAtomic::install");
        ast::NodePtr node{engine->makeNode(kind)};
        node->set(fields::Value, detail::parseInteger(item.text, "IntegerLiteralAtomic::install"));

        return node;
    });

    controller.engine().expression(kind,[](const ast::Node& node, runtime::RuntimeContext&){
        return runtime::Value::integer(node.integer(fields::Value));
    });
}

} // namespace novac::assets::atomic::literals
