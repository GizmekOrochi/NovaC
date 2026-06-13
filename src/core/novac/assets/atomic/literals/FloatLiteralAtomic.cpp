#include "novac/assets/atomic/literals/FloatLiteralAtomic.hpp"

#include "novac/assets/atomic/AtomicController.hpp"

#include <regex>
#include <stdexcept>
#include <utility>
#include <variant>

namespace novac::assets::atomic::literals {

namespace {

void validateSuffix(const TokenPattern& pattern, const token::Token& item, const std::string& owner) {
    if (pattern.mode == TokenPatternMode::Suffix) {
        if (item.suffix != pattern.suffix) {
            throw std::runtime_error(owner + ": expected suffix '" + pattern.suffix + "', got '" + item.suffix + "'");
        }

        return;
    }

    if (pattern.mode == TokenPatternMode::SuffixRegex) {
        if (!std::regex_match(item.suffix, std::regex{pattern.suffixPattern})) {
            throw std::runtime_error(owner + ": suffix '" + item.suffix + "' does not match regex '" + pattern.suffixPattern + "'");
        }
    }
}

} // namespace

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
    const TokenPattern pattern{pattern_};
    const std::string key{pattern.tokenKey.empty() ? pattern.token : pattern.tokenKey};

    controller.engine().prefix(domain, key, [kind, pattern, &controller](parser::ParserContext& context) {
        const token::Token item{context.consumeKind(token::Kind::Float)};
        validateSuffix(pattern, item, "FloatLiteralAtomic::install");
        ast::NodePtr node{controller.engine().makeNode(kind)};
        node->set("value", std::stod(item.text));

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