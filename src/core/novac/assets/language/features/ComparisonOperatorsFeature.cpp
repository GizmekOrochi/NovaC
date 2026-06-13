#include "novac/assets/language/features/ComparisonOperatorsFeature.hpp"

#include "novac/assets/language/LanguageOptionsController.hpp"

#include <stdexcept>
#include <utility>

namespace novac::language::features {

ComparisonOperatorsFeature::ComparisonOperatorsFeature(std::string expressionDomain, std::string binaryNodeKind)
    : expressionDomain_{std::move(expressionDomain)}, binaryNodeKind_{std::move(binaryNodeKind)} {
    if (expressionDomain_.empty() || binaryNodeKind_.empty()) {
        throw std::runtime_error("ComparisonOperatorsFeature::ComparisonOperatorsFeature: domain and binary node kind cannot be empty");
    }
}

LanguageFeatureInfo ComparisonOperatorsFeature::info() const {
    return {"language.comparison-operators", "0.1.0", "Comparison infix operators and runtime handlers", {"operator.comparison"}, {"expression.binary"}, {"language.binary-expression"}, {}, {}};
}

void ComparisonOperatorsFeature::install(LanguageOptionsController &language) const {
    const std::string domain{expressionDomain_};
    const std::string kind{binaryNodeKind_};

    const auto builder{[kind, &language](parser::ParserContext &, ast::NodePtr left, const token::Token &op, ast::NodePtr right) {
        ast::NodePtr node{language.engine().makeNode(kind)};
        node->set("op", op.text);
        node->set("left", left);
        node->set("right", right);
        return node;
    }};

    for (const std::string op : {std::string{"=="}, std::string{"!="}, std::string{"<="}, std::string{">="}, std::string{"<"}, std::string{">"}}) {
        language.symbol(op);
        language.infix(domain, op, 7, parser::Associativity::None, builder);
        language.binaryOperator(op, [op](const ast::Node &node, runtime::RuntimeContext &context) {
            const int left{context.eval(*node.child("left")).asInt()};
            const int right{context.eval(*node.child("right")).asInt()};

            if (op == "==") return runtime::Value::boolean(left == right);
            if (op == "!=") return runtime::Value::boolean(left != right);
            if (op == "<") return runtime::Value::boolean(left < right);
            if (op == "<=") return runtime::Value::boolean(left <= right);
            if (op == ">") return runtime::Value::boolean(left > right);
            if (op == ">=") return runtime::Value::boolean(left >= right);

            throw std::runtime_error("ComparisonOperatorsFeature::runtime: unsupported comparison operator '" + op + "'");
        });
    }
}

} // namespace novac::language::features
