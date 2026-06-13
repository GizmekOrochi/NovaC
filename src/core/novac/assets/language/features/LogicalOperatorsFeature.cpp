#include "novac/assets/language/features/LogicalOperatorsFeature.hpp"

#include "novac/assets/language/LanguageOptionsController.hpp"

#include <stdexcept>
#include <utility>

namespace novac::language::features {

LogicalOperatorsFeature::LogicalOperatorsFeature(std::string expressionDomain, std::string binaryNodeKind)
    : expressionDomain_{std::move(expressionDomain)}, binaryNodeKind_{std::move(binaryNodeKind)} {
    if (expressionDomain_.empty() || binaryNodeKind_.empty()) {
        throw std::runtime_error("LogicalOperatorsFeature::LogicalOperatorsFeature: domain and binary node kind cannot be empty");
    }
}

LanguageFeatureInfo LogicalOperatorsFeature::info() const {
    return {"language.logical-operators", "0.1.0", "Logical infix operators and runtime handlers", {"operator.logical"}, {"expression.binary"}, {"language.binary-expression"}, {}, {}};
}

void LogicalOperatorsFeature::install(LanguageOptionsController &language) const {
    const std::string domain{expressionDomain_};
    const std::string kind{binaryNodeKind_};

    const auto builder{[kind, &language](parser::ParserContext &, ast::NodePtr left, const token::Token &op, ast::NodePtr right) {
        ast::NodePtr node{language.engine().makeNode(kind)};
        node->set("op", op.text);
        node->set("left", left);
        node->set("right", right);
        return node;
    }};

    language.symbol("&&");
    language.symbol("||");
    language.infix(domain, "&&", 4, builder);
    language.infix(domain, "||", 3, builder);

    language.binaryOperator("&&", [](const ast::Node &node, runtime::RuntimeContext &context) {
        const bool left{context.eval(*node.child("left")).truthy()};

        if (!left) {
            return runtime::Value::boolean(false);
        }

        return runtime::Value::boolean(context.eval(*node.child("right")).truthy());
    });

    language.binaryOperator("||", [](const ast::Node &node, runtime::RuntimeContext &context) {
        const bool left{context.eval(*node.child("left")).truthy()};

        if (left) {
            return runtime::Value::boolean(true);
        }

        return runtime::Value::boolean(context.eval(*node.child("right")).truthy());
    });
}

} // namespace novac::language::features
