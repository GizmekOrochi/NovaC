#include "novac/assets/language/features/runtime/NumericArithmeticRuntimeFeature.hpp"
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

        if (op == "+") { return runtime::Value::integer(lhs + rhs); }
        if (op == "-") { return runtime::Value::integer(lhs - rhs); }
        if (op == "*") { return runtime::Value::integer(lhs * rhs); }
        if (op == "/") {
            if (rhs == 0) { throw std::runtime_error("NumericArithmeticRuntimeFeature::runtime: division by zero"); }
            return runtime::Value::integer(lhs / rhs);
        }
        if (op == "%") {
            if (rhs == 0) { throw std::runtime_error("NumericArithmeticRuntimeFeature::runtime: modulo by zero"); }
            return runtime::Value::integer(lhs % rhs);
        }
    }

    const double lhs{left.asFloat()};
    const double rhs{right.asFloat()};

    if (op == "+") { return runtime::Value::floating(lhs + rhs); }
    if (op == "-") { return runtime::Value::floating(lhs - rhs); }
    if (op == "*") { return runtime::Value::floating(lhs * rhs); }
    if (op == "/") {
        if (rhs == 0.0) { throw std::runtime_error("NumericArithmeticRuntimeFeature::runtime: division by zero"); }
        return runtime::Value::floating(lhs / rhs);
    }
    if (op == "%") {
        if (rhs == 0.0) { throw std::runtime_error("NumericArithmeticRuntimeFeature::runtime: modulo by zero"); }
        return runtime::Value::floating(std::fmod(lhs, rhs));
    }

    throw std::runtime_error("NumericArithmeticRuntimeFeature::runtime: unsupported arithmetic operator '" + op + "'");
}

runtime::Value evaluateBinary(const ast::Node &node, runtime::RuntimeContext &context, const std::string &op) {
    const runtime::Value left{context.eval(*node.child("left"))};
    const runtime::Value right{context.eval(*node.child("right"))};

    return numericBinary(left, right, op);
}

} // namespace

NumericArithmeticRuntimeFeature::NumericArithmeticRuntimeFeature(std::string binaryNodeKind)
    : binaryNodeKind_{std::move(binaryNodeKind)} {
    if (binaryNodeKind_.empty()) {
        throw std::runtime_error("NumericArithmeticRuntimeFeature::NumericArithmeticRuntimeFeature: binary node kind cannot be empty");
    }
}

LanguageFeatureInfo NumericArithmeticRuntimeFeature::info() const {
    return {"language.runtime.numeric-arithmetic", "0.2.0", "Runtime numeric arithmetic with int/float promotion", {"runtime.operator.arithmetic", "runtime.numeric.promotion"}, {"expression.binary", "runtime.value.integer"}, {"language.expression.binary"}, {}, {}};
}

void NumericArithmeticRuntimeFeature::install(LanguageOptionsController &language) const {
    static_cast<void>(binaryNodeKind_);
    const std::array<std::string, 5> operators{"+", "-", "*", "/", "%"};

    for (const std::string &op : operators) {
        language.binaryOperator(op, [op](const ast::Node &node, runtime::RuntimeContext &context) {
            return evaluateBinary(node, context, op);
        });
    }
}

} // namespace novac::language::features
