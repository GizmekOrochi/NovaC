#include "novac/assets/atomic/literals/FloatLiteralAtomic.hpp"

#include "novac/assets/atomic/AtomicController.hpp"
#include "LiteralParsing.hpp"

#include <stdexcept>
#include <utility>
#include <variant>

namespace novac::assets::atomic::literals {


FloatLiteralAtomic::FloatLiteralAtomic(
    std::string nodeKind,
    TokenPattern pattern)
    : nodeKind_{std::move(nodeKind)},
      pattern_{std::move(pattern)}
{
    if (nodeKind_.empty()) {
        throw std::runtime_error(
            "FloatLiteralAtomic::FloatLiteralAtomic: node kind cannot be empty");
    }
}

LiteralInfo FloatLiteralAtomic::info() const {
    return {"core.literal.float", "0.1.0", "Floating-point literal atomic", nodeKind_, pattern_, {"literal.float", "expression.atom"}, {}};
}

void FloatLiteralAtomic::install(AtomicController& controller) const {
    controller.registerPattern(pattern_);

    controller.engine().node({
        .kind = nodeKind_,
        .fields = {
            {
                .name = "value",
                .kind = ast::FieldKind::Float,
                .required = true
            }
        },
        .traits = {"expr", "literal"},
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
        node->set("value", detail::parseFloat(item.text, "FloatLiteralAtomic::install"));

        return node;
    });

    controller.engine().expression(kind, [](const ast::Node& node, runtime::RuntimeContext&) {
        const ast::Field& field{
            node.field("value")
        };

        return runtime::Value::floating(
            std::get<double>(field));
    });
}

} // namespace novac::assets::atomic::literals