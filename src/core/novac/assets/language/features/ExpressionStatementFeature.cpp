#include "novac/assets/language/features/ExpressionStatementFeature.hpp"

#include "novac/assets/language/LanguageOptionsController.hpp"

#include <stdexcept>
#include <utility>

namespace novac::language::features {

ExpressionStatementFeature::ExpressionStatementFeature(std::string statementDomain, std::string expressionDomain, std::string nodeKind)
    : statementDomain_{std::move(statementDomain)}, expressionDomain_{std::move(expressionDomain)}, nodeKind_{std::move(nodeKind)} {
    if (statementDomain_.empty() || expressionDomain_.empty() || nodeKind_.empty()) {
        throw std::runtime_error("ExpressionStatementFeature::ExpressionStatementFeature: domains and node kind cannot be empty");
    }
}

LanguageFeatureInfo ExpressionStatementFeature::info() const {
    return {"language.expression-statement", "0.1.0", "Expression statement wrapper", {"statement.expression"}, {"expression.atom"}, {}, {}, {}};
}

void ExpressionStatementFeature::install(LanguageOptionsController &language) const {
    const std::string statementDomain{statementDomain_};
    const std::string expressionDomain{expressionDomain_};
    const std::string kind{nodeKind_};

    language.symbol(";");
    language.node({
        .kind = kind,
        .fields = {{.name = "expr", .kind = ast::FieldKind::Node, .required = true, .allowedNodeTraits = {"expr"}}},
        .traits = {"stmt"},
        .doc = "Expression statement"
    });

    language.fallback(statementDomain, [kind, expressionDomain, &language](parser::ParserContext &context) {
        ast::NodePtr expression{context.parse(expressionDomain)};

        if (context.check(";")) {
            context.consume(";");
        }

        ast::NodePtr node{language.engine().makeNode(kind)};
        node->set("expr", expression);
        return node;
    });

    language.statement(kind, [](const ast::Node &node, runtime::RuntimeContext &context) {
        static_cast<void>(context.eval(*node.child("expr")));
    });
}

} // namespace novac::language::features
