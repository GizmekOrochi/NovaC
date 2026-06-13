#include "novac/assets/language/features/literals/IntegerLiteralFeature.hpp"
#include "novac/assets/language/LanguageOptionsController.hpp"

#include <stdexcept>
#include <utility>

namespace novac::language::features {

IntegerLiteralFeature::IntegerLiteralFeature(std::string expressionDomain, std::string nodeKind)
    : expressionDomain_{std::move(expressionDomain)}, nodeKind_{std::move(nodeKind)} {
    if (expressionDomain_.empty() || nodeKind_.empty()) {
        throw std::runtime_error("IntegerLiteralFeature::IntegerLiteralFeature: domain and node kind cannot be empty");
    }
}

LanguageFeatureInfo IntegerLiteralFeature::info() const {
    return {"language.literal.integer", "0.2.0", "Integer literal syntax and runtime value", {"literal.integer", "expression.atom", "runtime.value.integer"}, {}, {}, {}, {}};
}

void IntegerLiteralFeature::install(LanguageOptionsController &language) const {
    const std::string domain{expressionDomain_};
    const std::string kind{nodeKind_};

    language.node({.kind = kind, .fields = {{.name = "value", .kind = ast::FieldKind::Int, .required = true}}, .traits = {"expr", "literal", "numeric"}, .doc = "Integer literal expression"});
    language.prefix(domain, "$int", [kind, &language](parser::ParserContext &context) {
        const token::Token token{context.consumeKind(token::Kind::Integer)};
        ast::NodePtr node{language.engine().makeNode(kind)};
        node->set("value", std::stoi(token.text));
        return node;
    });
    language.expression(kind, [](const ast::Node &node, runtime::RuntimeContext &) { return runtime::Value::integer(node.integer("value")); });
}

} // namespace novac::language::features
