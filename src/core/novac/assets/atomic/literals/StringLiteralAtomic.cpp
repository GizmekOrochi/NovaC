#include "novac/assets/atomic/literals/StringLiteralAtomic.hpp"
#include "novac/assets/atomic/AtomicIds.hpp"
#include "novac/assets/AssetTraits.hpp"

#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/assets/atomic/literals/LiteralParsing.hpp"

#include <stdexcept>
#include <utility>

namespace novac::assets::atomic::literals {


/**
 * @brief Constructs a `StringLiteralAtomic` instance.
 *
 * @param nodeKind Value supplied for `nodeKind`.
 * @param pattern Value supplied for `pattern`.
 */
StringLiteralAtomic::StringLiteralAtomic(std::string nodeKind, TokenPattern pattern)
    : nodeKind_{std::move(nodeKind)}, pattern_{std::move(pattern)} {
    if (nodeKind_.empty())
        throw std::runtime_error("StringLiteralAtomic::StringLiteralAtomic: node kind cannot be empty");
}

/**
 * @brief Implements the `info` operation.
 *
 * @return Value produced by the operation.
 */
LiteralInfo StringLiteralAtomic::info() const {
    return {"core.literal.string", "0.1.0", "String literal atomic", nodeKind_, pattern_, {"literal.string", "expression.atom"}, {}};
}

/**
 * @brief Installs the behavior provided by `install`.
 *
 * @param controller Value supplied for `controller`.
 */
void StringLiteralAtomic::install(AtomicController& controller) const {
    controller.registerPattern(pattern_);

    controller.engine().node({
        .kind = nodeKind_,
        .fields = {
            {
                .name = fields::Value.value,
                .kind = ast::FieldKind::String,
                .required = true
            }
        },
        .traits = {novac::assets::traits::Expression, novac::assets::traits::Literal},
        .doc = "String literal expression"
    });

    const std::string domain{controller.expressionDomain()};
    const std::string kind{nodeKind_};
    const auto preparedPattern{detail::prepareSuffixPattern(pattern_, "StringLiteralAtomic::install")};
    const TokenPattern &pattern{preparedPattern.pattern};
    const std::string key{pattern.tokenKey.empty() ? pattern.token : pattern.tokenKey};
    auto *const engine{&controller.engine()};

    controller.engine().prefix( domain, key,[kind, preparedPattern, engine](parser::ParserContext& context){
            const token::Token item{context.consumeKind(token::Kind::String)};
            detail::validateSuffix(preparedPattern, item, "StringLiteralAtomic::install");
            ast::NodePtr node{engine->makeNode(kind)};
            node->set(fields::Value, item.text);

            return node;
        });

    controller.engine().expression(kind, [](const ast::Node& node, runtime::RuntimeContext&) {
        return runtime::Value::string(node.str(fields::Value));
    });
}

} // namespace novac::assets::atomic::literals
