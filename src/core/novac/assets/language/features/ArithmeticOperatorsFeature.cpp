#include "novac/assets/language/features/ArithmeticOperatorsFeature.hpp"

#include "novac/assets/language/LanguageOptionsController.hpp"

#include <array>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace novac::language::features {

namespace {

bool isIntegerValue(const runtime::Value &value) {
    try {
        static_cast<void>(value.asInt());
        return true;
    } catch (const std::runtime_error &) {
        return false;
    }
}

runtime::Value numericBinary(const runtime::Value &left, const runtime::Value &right, const std::string &op) {
    const bool integerOperands{isIntegerValue(left) && isIntegerValue(right)};

    if (integerOperands) {
        const int lhs{left.asInt()};
        const int rhs{right.asInt()};

        if (op == "+") {
            return runtime::Value::integer(lhs + rhs);
        }

        if (op == "-") {
            return runtime::Value::integer(lhs - rhs);
        }

        if (op == "*") {
            return runtime::Value::integer(lhs * rhs);
        }

        if (op == "/") {
            if (rhs == 0) {
                throw std::runtime_error("ArithmeticOperatorsFeature::numericBinary: division by zero");
            }

            return runtime::Value::integer(lhs / rhs);
        }

        if (op == "%") {
            if (rhs == 0) {
                throw std::runtime_error("ArithmeticOperatorsFeature::numericBinary: modulo by zero");
            }

            return runtime::Value::integer(lhs % rhs);
        }
    }

    const double lhs{left.asFloat()};
    const double rhs{right.asFloat()};

    if (op == "+") {
        return runtime::Value::floating(lhs + rhs);
    }

    if (op == "-") {
        return runtime::Value::floating(lhs - rhs);
    }

    if (op == "*") {
        return runtime::Value::floating(lhs * rhs);
    }

    if (op == "/") {
        if (rhs == 0.0) {
            throw std::runtime_error("ArithmeticOperatorsFeature::numericBinary: division by zero");
        }

        return runtime::Value::floating(lhs / rhs);
    }

    if (op == "%") {
        if (rhs == 0.0) {
            throw std::runtime_error("ArithmeticOperatorsFeature::numericBinary: modulo by zero");
        }

        return runtime::Value::floating(std::fmod(lhs, rhs));
    }

    throw std::runtime_error("ArithmeticOperatorsFeature::numericBinary: unsupported arithmetic operator '" + op + "'");
}

runtime::Value evaluateBinary(const ast::Node &node, runtime::RuntimeContext &context, const std::string &op) {
    const runtime::Value left{context.eval(*node.child("left"))};
    const runtime::Value right{context.eval(*node.child("right"))};

    return numericBinary(left, right, op);
}

} // namespace

ArithmeticOperatorsFeature::ArithmeticOperatorsFeature(std::string expressionDomain, std::string binaryNodeKind)
    : expressionDomain_{std::move(expressionDomain)}, binaryNodeKind_{std::move(binaryNodeKind)} {
    if (expressionDomain_.empty() || binaryNodeKind_.empty()) {
        throw std::runtime_error("ArithmeticOperatorsFeature::ArithmeticOperatorsFeature: domain and binary node kind cannot be empty");
    }
}

LanguageFeatureInfo ArithmeticOperatorsFeature::info() const {
    return {"language.arithmetic-operators", "0.2.0", "Arithmetic infix operators with numeric promotion", {"operator.arithmetic"}, {"expression.binary"}, {"language.binary-expression"}, {}, {}};
}

void ArithmeticOperatorsFeature::install(LanguageOptionsController &language) const {
    const std::string domain{expressionDomain_};
    const std::string kind{binaryNodeKind_};

    const auto builder{
        [kind, &language](parser::ParserContext &, ast::NodePtr left, const token::Token &op, ast::NodePtr right) {
            ast::NodePtr node{language.engine().makeNode(kind)};
            node->set("op", op.text);
            node->set("left", left);
            node->set("right", right);
            return node;
        }
    };

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

    const std::array<std::string, 5> operators{"+", "-", "*", "/", "%"};

    for (const std::string &op : operators) {
        language.binaryOperator(op, [op](const ast::Node &node, runtime::RuntimeContext &context) {
            return evaluateBinary(node, context, op);
        });
    }
}

} // namespace novac::language::features