#include "novac/assets/language/features/UnaryExpressionFeature.hpp"

#include "novac/assets/language/LanguageOptionsController.hpp"

#include <stdexcept>
#include <utility>

namespace novac::language::features {

UnaryExpressionFeature::UnaryExpressionFeature(std::string expressionDomain, std::string nodeKind)
    : expressionDomain_{std::move(expressionDomain)}, nodeKind_{std::move(nodeKind)} {
    if (expressionDomain_.empty() || nodeKind_.empty()) {
        throw std::runtime_error("UnaryExpressionFeature::UnaryExpressionFeature: domain and node kind cannot be empty");
    }
}

LanguageFeatureInfo UnaryExpressionFeature::info() const {
    return {"language.unary-expression", "0.1.0", "Unary expression AST and runtime evaluation", {"expression.unary"}, {"expression.atom"}, {}, {}, {}};
}

void UnaryExpressionFeature::install(LanguageOptionsController &language) const {
    const std::string domain{expressionDomain_};
    const std::string kind{nodeKind_};

    language.node({
        .kind = kind,
        .fields = {
            {.name = "op", .kind = ast::FieldKind::String, .required = true},
            {.name = "expr", .kind = ast::FieldKind::Node, .required = true, .allowedNodeTraits = {"expr"}}
        },
        .traits = {"expr"},
        .doc = "Unary expression"
    });

    const auto builder{[domain, kind, &language](parser::ParserContext &context) {
        const token::Token op{context.advance()};
        ast::NodePtr operand{context.parse(domain, 30)};
        ast::NodePtr node{language.engine().makeNode(kind)};
        node->set("op", op.text);
        node->set("expr", operand);
        return node;
    }};

    language.symbol("-");
    language.symbol("!");
    language.prefix(domain, "-", builder);
    language.prefix(domain, "!", builder);

    language.expression(kind, [](const ast::Node &node, runtime::RuntimeContext &context) {
        const std::string op{node.str("op")};

        if (op == "-") {
            return runtime::Value::integer(-context.eval(*node.child("expr")).asInt());
        }

        if (op == "!") {
            return runtime::Value::boolean(!context.eval(*node.child("expr")).truthy());
        }

        throw std::runtime_error("UnaryExpressionFeature::runtime: unsupported unary operator '" + op + "'");
    });
}

} // namespace novac::language::features
