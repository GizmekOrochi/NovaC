#include "novac/assets/language/features/literals/FloatingLiteralFeature.hpp"
#include "novac/assets/language/LanguageOptionsController.hpp"

#include <stdexcept>
#include <utility>
#include <variant>

namespace novac::language::features {

FloatingLiteralFeature::FloatingLiteralFeature(std::string expressionDomain, std::string nodeKind)
    : expressionDomain_{std::move(expressionDomain)}, nodeKind_{std::move(nodeKind)} {
    if (expressionDomain_.empty() || nodeKind_.empty()) {
        throw std::runtime_error("FloatingLiteralFeature::FloatingLiteralFeature: domain and node kind cannot be empty");
    }
}

LanguageFeatureInfo FloatingLiteralFeature::info() const {
    return {"language.literal.float", "0.2.0", "Floating-point literal syntax and runtime value", {"literal.float", "expression.atom", "runtime.value.float"}, {}, {}, {}, {}};
}

void FloatingLiteralFeature::install(LanguageOptionsController &language) const {
    const std::string domain{expressionDomain_};
    const std::string kind{nodeKind_};

    language.node({.kind = kind, .fields = {{.name = "value", .kind = ast::FieldKind::Float, .required = true}}, .traits = {"expr", "literal", "numeric"}, .doc = "Floating-point literal expression"});
    language.prefix(domain, "$float", [kind, &language](parser::ParserContext &context) {
        const token::Token token{context.consumeKind(token::Kind::Float)};
        ast::NodePtr node{language.engine().makeNode(kind)};
        node->set("value", std::stod(token.text));
        return node;
    });
    language.expression(kind, [](const ast::Node &node, runtime::RuntimeContext &) {
        return runtime::Value::floating(std::get<double>(node.field("value")));
    });
}

} // namespace novac::language::features
