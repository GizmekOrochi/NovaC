#include "novac/assets/language/features/expressions/IdentifierExpressionFeature.hpp"
#include "novac/assets/language/LanguageOptionsController.hpp"

#include <stdexcept>
#include <utility>

namespace novac::language::features {

IdentifierExpressionFeature::IdentifierExpressionFeature(std::string expressionDomain, std::string nodeKind)
    : expressionDomain_{std::move(expressionDomain)}, nodeKind_{std::move(nodeKind)} {
    if (expressionDomain_.empty() || nodeKind_.empty()) {
        throw std::runtime_error("IdentifierExpressionFeature::IdentifierExpressionFeature: domain and node kind cannot be empty");
    }
}

LanguageFeatureInfo IdentifierExpressionFeature::info() const {
    return {"language.expression.identifier", "0.2.0", "Identifier expression parsing and runtime lookup", {"expression.identifier", "expression.atom"}, {}, {}, {}, {}};
}

void IdentifierExpressionFeature::install(LanguageOptionsController &language) const {
    const std::string domain{expressionDomain_};
    const std::string kind{nodeKind_};

    language.node({.kind = kind, .fields = {{.name = "name", .kind = ast::FieldKind::String, .required = true}}, .traits = {"expr"}, .doc = "Identifier expression"});
    language.prefix(domain, "$identifier", [kind, &language](parser::ParserContext &context) {
        const token::Token token{context.consumeKind(token::Kind::Identifier)};
        ast::NodePtr node{language.engine().makeNode(kind)};
        node->set("name", token.text);
        return node;
    });
    language.expression(kind, [](const ast::Node &node, runtime::RuntimeContext &context) {
        const runtime::Value *value{context.env().resolve(node.str("name"))};
        if (!value) {
            throw std::runtime_error("IdentifierExpressionFeature::runtime: undefined variable '" + node.str("name") + "'");
        }
        return *value;
    });
}

} // namespace novac::language::features
