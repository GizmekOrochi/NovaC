#include "novac/assets/language/features/BooleanLiteralFeature.hpp"

#include "novac/assets/language/LanguageOptionsController.hpp"

#include <stdexcept>
#include <utility>
#include <variant>

namespace novac::language::features {

BooleanLiteralFeature::BooleanLiteralFeature(std::string expressionDomain, std::string nodeKind)
    : expressionDomain_{std::move(expressionDomain)}, nodeKind_{std::move(nodeKind)} {
    if (expressionDomain_.empty() || nodeKind_.empty()) {
        throw std::runtime_error("BooleanLiteralFeature::BooleanLiteralFeature: domain and node kind cannot be empty");
    }
}

LanguageFeatureInfo BooleanLiteralFeature::info() const {
    return {"language.boolean-literal", "0.1.0", "Boolean literal syntax and runtime evaluation", {"literal.boolean", "expression.atom"}, {}, {}, {}, {}};
}

void BooleanLiteralFeature::install(LanguageOptionsController &language) const {
    const std::string domain{expressionDomain_};
    const std::string kind{nodeKind_};

    language.keyword("true");
    language.keyword("false");
    language.node({
        .kind = kind,
        .fields = {{.name = "value", .kind = ast::FieldKind::Bool, .required = true}},
        .traits = {"expr", "literal"},
        .doc = "Boolean literal expression"
    });

    const auto makeBoolean{[kind, &language](parser::ParserContext &context) {
        const token::Token token{context.consumeKind(token::Kind::Keyword)};
        ast::NodePtr node{language.engine().makeNode(kind)};
        node->set("value", token.text == "true");
        return node;
    }};

    language.prefix(domain, "true", makeBoolean);
    language.prefix(domain, "false", makeBoolean);

    language.expression(kind, [](const ast::Node &node, runtime::RuntimeContext &) {
        const ast::Field &value{node.field("value")};
        return runtime::Value::boolean(std::get<bool>(value));
    });
}

} // namespace novac::language::features
