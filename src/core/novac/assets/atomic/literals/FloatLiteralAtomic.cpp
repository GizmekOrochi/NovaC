#include "novac/assets/atomic/literals/FloatLiteralAtomic.hpp"
#include "novac/assets/atomic/AtomicIds.hpp"
#include "novac/assets/AssetTraits.hpp"

#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/assets/atomic/literals/LiteralParsing.hpp"

#include <stdexcept>
#include <utility>
#include <variant>

namespace novac::assets::atomic::literals {


/**
 * @brief Constructs a `FloatLiteralAtomic` instance.
 *
 * @param nodeKind Value supplied for `nodeKind`.
 * @param pattern Value supplied for `pattern`.
 */
FloatLiteralAtomic::FloatLiteralAtomic(std::string nodeKind, TokenPattern pattern)
    : nodeKind_{std::move(nodeKind)}, pattern_{std::move(pattern)} {
    if (nodeKind_.empty())
        throw std::runtime_error("FloatLiteralAtomic::FloatLiteralAtomic: node kind cannot be empty");
}

/**
 * @brief Implements the `info` operation.
 *
 * @return Value produced by the operation.
 */
LiteralInfo FloatLiteralAtomic::info() const {
    return {"core.literal.float", "0.1.0", "Floating-point literal atomic", nodeKind_, pattern_, {"literal.float", "expression.atom"}, {}};
}

/**
 * @brief Installs the behavior provided by `install`.
 *
 * @param controller Value supplied for `controller`.
 */
void FloatLiteralAtomic::install(AtomicController& controller) const {
    controller.registerPattern(pattern_);

    controller.engine().node({
        .kind = nodeKind_,
        .fields = {
            {
                .name = fields::Value.value,
                .kind = ast::FieldKind::Float,
                .required = true
            }
        },
        .traits = {novac::assets::traits::Expression, novac::assets::traits::Literal},
        .doc = "Floating-point literal expression"
    });

    const std::string domain{controller.expressionDomain()};
    const std::string kind{nodeKind_};
    const auto preparedPattern{detail::prepareSuffixPattern(pattern_, "FloatLiteralAtomic::install")};
    const TokenPattern &pattern{preparedPattern.pattern};
    const std::string key{pattern.tokenKey.empty() ? pattern.token : pattern.tokenKey};
    auto *const engine{&controller.engine()};

    controller.engine().prefix(domain, key, [kind, preparedPattern, engine](parser::ParserContext& context) {
        const token::Token item{context.consumeKind(token::Kind::Float)};
        detail::validateSuffix(preparedPattern, item, "FloatLiteralAtomic::install");
        ast::NodePtr node{engine->makeNode(kind)};
        node->set(fields::Value, detail::parseFloat(item.text, "FloatLiteralAtomic::install"));

        return node;
    });

    controller.engine().expression(kind, [](const ast::Node& node, runtime::RuntimeContext&) {
        const ast::Field& field{node.field(fields::Value)};

        return runtime::Value::floating(
            std::get<double>(field));
    });
}

} // namespace novac::assets::atomic::literals
