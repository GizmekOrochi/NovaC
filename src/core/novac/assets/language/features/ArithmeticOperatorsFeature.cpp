#include "novac/assets/language/features/ArithmeticOperatorsFeature.hpp"
#include "novac/assets/language/LanguageOptionsController.hpp"

#include <stdexcept>
#include <utility>

namespace novac::language::features {

namespace {

runtime::Value intBinary(const ast::Node &node, runtime::RuntimeContext &context, const std::string &op) {
    const int left{context.eval(*node.child("left")).asInt()};
    const int right{context.eval(*node.child("right")).asInt()};

    if (op == "+") {
        return runtime::Value::integer(left + right);
    }

    if (op == "-") {
        return runtime::Value::integer(left - right);
    }

    if (op == "*") {
        return runtime::Value::integer(left * right);
    }

    if (op == "/") {
        if (right == 0) {
            throw std::runtime_error("ArithmeticOperatorsFeature::runtime: division by zero");
        }

        return runtime::Value::integer(left / right);
    }

    if (op == "%") {
        if (right == 0) {
            throw std::runtime_error("ArithmeticOperatorsFeature::runtime: modulo by zero");
        }

        return runtime::Value::integer(left % right);
    }

    throw std::runtime_error("ArithmeticOperatorsFeature::runtime: unsupported arithmetic operator '" + op + "'");
}

} // namespace

ArithmeticOperatorsFeature::ArithmeticOperatorsFeature(std::string expressionDomain, std::string binaryNodeKind)
    : expressionDomain_{std::move(expressionDomain)}, binaryNodeKind_{std::move(binaryNodeKind)} {
    if (expressionDomain_.empty() || binaryNodeKind_.empty()) {
        throw std::runtime_error("ArithmeticOperatorsFeature::ArithmeticOperatorsFeature: domain and binary node kind cannot be empty");
    }
}

LanguageFeatureInfo ArithmeticOperatorsFeature::info() const {
    return {"language.arithmetic-operators", "0.1.0", "Arithmetic infix operators and runtime handlers", {"operator.arithmetic"}, {"expression.binary"}, {"language.binary-expression"}, {}, {}};
}

void ArithmeticOperatorsFeature::install(LanguageOptionsController &language) const {
    const std::string domain{expressionDomain_};
    const std::string kind{binaryNodeKind_};

    const auto builder{[kind, &language](parser::ParserContext &, ast::NodePtr left, const token::Token &op, ast::NodePtr right) {
        ast::NodePtr node{language.engine().makeNode(kind)};
        node->set("op", op.text);
        node->set("left", left);
        node->set("right", right);
        return node;
    }};

    language.symbol("+");
    language.symbol("-");
    language.symbol("*");
    language.symbol("/");
    language.symbol("%");

    language.infix(domain, "+", 10, builder);
    language.infix(domain, "-", 10, builder);
    language.infix(domain, "*", 20, builder);
    language.infix(domain, "/", 20, builder);
    language.infix(domain, "%", 20, builder);

    for (const std::string op : {std::string{"+"}, std::string{"-"}, std::string{"*"}, std::string{"/"}, std::string{"%"}}) {
        language.binaryOperator(op, [op](const ast::Node &node, runtime::RuntimeContext &context) {
            return intBinary(node, context, op);
        });
    }
}

} // namespace novac::language::features
