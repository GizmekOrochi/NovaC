#include "novac/assets/language/features/StringLiteralFeature.hpp"

#include "novac/assets/language/LanguageOptionsController.hpp"

#include <stdexcept>
#include <utility>

namespace novac::language::features {

StringLiteralFeature::StringLiteralFeature(std::string expressionDomain, std::string nodeKind)
    : expressionDomain_{std::move(expressionDomain)}, nodeKind_{std::move(nodeKind)} {
    if (expressionDomain_.empty() || nodeKind_.empty()) {
        throw std::runtime_error("StringLiteralFeature::StringLiteralFeature: domain and node kind cannot be empty");
    }
}

LanguageFeatureInfo StringLiteralFeature::info() const {
    return {"language.string-literal", "0.1.0", "String literal syntax and runtime evaluation", {"literal.string", "expression.atom"}, {}, {}, {}, {}};
}

void StringLiteralFeature::install(LanguageOptionsController &language) const {
    const std::string domain{expressionDomain_};
    const std::string kind{nodeKind_};

    language.node({
        .kind = kind,
        .fields = {{.name = "value", .kind = ast::FieldKind::String, .required = true}},
        .traits = {"expr", "literal"},
        .doc = "String literal expression"
    });

    language.prefix(domain, "$string", [kind, &language](parser::ParserContext &context) {
        const token::Token token{context.consumeKind(token::Kind::String)};
        ast::NodePtr node{language.engine().makeNode(kind)};
        node->set("value", token.text);
        return node;
    });

    language.expression(kind, [](const ast::Node &node, runtime::RuntimeContext &) {
        return runtime::Value::string(node.str("value"));
    });
}

} // namespace novac::language::features
