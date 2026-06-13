#include "novac/assets/atomic/literals/IntegerLiteralAtomic.hpp"

#include "novac/assets/atomic/AtomicController.hpp"

#include <regex>
#include <stdexcept>
#include <utility>

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

IntegerLiteralAtomic::IntegerLiteralAtomic(std::string nodeKind, TokenPattern pattern)
    : nodeKind_{std::move(nodeKind)}, pattern_{std::move(pattern)} {
    if (nodeKind_.empty()) {
        throw std::runtime_error(
            "IntegerLiteralAtomic::IntegerLiteralAtomic: node kind cannot be empty");
    }
}

LiteralInfo IntegerLiteralAtomic::info() const {
    return {"core.literal.integer", "0.1.0", "Integer literal atomic", nodeKind_, pattern_, {"literal.integer", "expression.atom"}, {}};
}

void IntegerLiteralAtomic::install(AtomicController& controller) const {
    controller.registerPattern(pattern_);

    controller.engine().node({
        .kind = nodeKind_,
        .fields = {
            {
                .name = "value",
                .kind = ast::FieldKind::Int,
                .required = true
            }
        },
        .traits = {"expr", "literal"},
        .doc = "Integer literal expression"
    });

    const std::string domain{controller.expressionDomain()};
    const std::string kind{nodeKind_};
    const TokenPattern pattern{pattern_};
    const std::string key{pattern.tokenKey.empty() ? pattern.token : pattern.tokenKey};

    controller.engine().prefix(domain, key, [kind, pattern, &controller](parser::ParserContext& context) {
        const token::Token item{context.consumeKind(token::Kind::Integer)};
        validateSuffix(pattern, item, "IntegerLiteralAtomic::install");
        ast::NodePtr node{controller.engine().makeNode(kind)};
        node->set("value", std::stoi(item.text));

        return node;
    });

    controller.engine().expression(kind,[](const ast::Node& node, runtime::RuntimeContext&){
        return runtime::Value::integer(node.integer("value"));
    });
}

} // namespace novac::assets::atomic::literals