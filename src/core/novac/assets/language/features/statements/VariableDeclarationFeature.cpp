#include "novac/assets/language/features/statements/VariableDeclarationFeature.hpp"
#include "novac/assets/language/LanguageOptionsController.hpp"

#include <stdexcept>
#include <utility>

namespace novac::language::features {

VariableDeclarationFeature::VariableDeclarationFeature(std::string statementDomain, std::string expressionDomain, std::string nodeKind)
    : statementDomain_{std::move(statementDomain)}, expressionDomain_{std::move(expressionDomain)}, nodeKind_{std::move(nodeKind)} {
    if (statementDomain_.empty() || expressionDomain_.empty() || nodeKind_.empty()) {
        throw std::runtime_error("VariableDeclarationFeature::VariableDeclarationFeature: domains and node kind cannot be empty");
    }
}

LanguageFeatureInfo VariableDeclarationFeature::info() const {
    return {"language.statement.variable-declaration", "0.2.0", "let-style variable declaration", {"statement.variable-declaration"}, {"expression.atom"}, {}, {}, {}};
}

void VariableDeclarationFeature::install(LanguageOptionsController &language) const {
    const std::string statementDomain{statementDomain_};
    const std::string expressionDomain{expressionDomain_};
    const std::string kind{nodeKind_};

    language.keyword("let");
    language.symbol("=");
    language.symbol(";");
    language.node({
        .kind = kind,
        .fields = {
            {.name = "name", .kind = ast::FieldKind::String, .required = true},
            {.name = "value", .kind = ast::FieldKind::Node, .required = true, .allowedNodeTraits = {"expr"}}
        },
        .traits = {"stmt", "decl"},
        .doc = "Variable declaration statement"
    });

    language.parseRule(statementDomain, "let", [kind, expressionDomain, &language](parser::ParserContext &context) {
        context.consume("let");
        const token::Token name{context.consumeKind(token::Kind::Identifier)};
        context.consume("=");
        ast::NodePtr value{context.parse(expressionDomain)};

        if (context.check(";")) {
            context.consume(";");
        }

        ast::NodePtr node{language.engine().makeNode(kind)};
        node->set("name", name.text);
        node->set("value", value);
        return node;
    });

    language.statement(kind, [](const ast::Node &node, runtime::RuntimeContext &context) {
        runtime::Value value{context.eval(*node.child("value"))};

        if (!context.env().define(node.str("name"), std::move(value))) {
            throw std::runtime_error("VariableDeclarationFeature::runtime: duplicate variable '" + node.str("name") + "'");
        }
    });
}

} // namespace novac::language::features
