#include "novac/assets/atomic/literals/StringLiteralAtomic.hpp"

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

StringLiteralAtomic::StringLiteralAtomic(std::string nodeKind, TokenPattern pattern)
    : nodeKind_{std::move(nodeKind)}, pattern_{std::move(pattern)} {
    if (nodeKind_.empty()) {
        throw std::runtime_error("StringLiteralAtomic::StringLiteralAtomic: node kind cannot be empty");
    }
}

LiteralInfo StringLiteralAtomic::info() const {
    return {"core.literal.string", "0.1.0", "String literal atomic", nodeKind_, pattern_, {"literal.string", "expression.atom"}, {}};
}

void StringLiteralAtomic::install(AtomicController& controller) const {
    controller.registerPattern(pattern_);

    controller.engine().node({
        .kind = nodeKind_,
        .fields = {
            {
                .name = "value",
                .kind = ast::FieldKind::String,
                .required = true
            }
        },
        .traits = {"expr", "literal"},
        .doc = "String literal expression"
    });

    const std::string domain{controller.expressionDomain()};
    const std::string kind{nodeKind_};
    const TokenPattern pattern{pattern_};
    const std::string key{pattern.tokenKey.empty() ? pattern.token : pattern.tokenKey};

    controller.engine().prefix( domain, key,[kind, pattern, &controller](parser::ParserContext& context){
            const token::Token item{context.consumeKind(token::Kind::String)};
            validateSuffix(pattern, item, "StringLiteralAtomic::install");
            ast::NodePtr node{controller.engine().makeNode(kind)};
            node->set("value", item.text);

            return node;
        });

    controller.engine().expression(kind, [](const ast::Node& node, runtime::RuntimeContext&) {
        return runtime::Value::string(node.str("value"));
    });
}

} // namespace novac::assets::atomic::literals