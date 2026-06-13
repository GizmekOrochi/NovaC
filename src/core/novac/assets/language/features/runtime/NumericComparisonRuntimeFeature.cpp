#include "novac/assets/language/features/runtime/NumericComparisonRuntimeFeature.hpp"
#include "novac/assets/language/LanguageOptionsController.hpp"

#include <array>
#include <stdexcept>
#include <utility>

namespace novac::language::features {

namespace {

runtime::Value compareNumeric(const runtime::Value &left, const runtime::Value &right, const std::string &op) {
    const double lhs{left.asFloat()};
    const double rhs{right.asFloat()};

    if (op == "==") { return runtime::Value::boolean(lhs == rhs); }
    if (op == "!=") { return runtime::Value::boolean(lhs != rhs); }
    if (op == "<") { return runtime::Value::boolean(lhs < rhs); }
    if (op == "<=") { return runtime::Value::boolean(lhs <= rhs); }
    if (op == ">") { return runtime::Value::boolean(lhs > rhs); }
    if (op == ">=") { return runtime::Value::boolean(lhs >= rhs); }

    throw std::runtime_error("NumericComparisonRuntimeFeature::runtime: unsupported comparison operator '" + op + "'");
}

runtime::Value evaluateBinary(const ast::Node &node, runtime::RuntimeContext &context, const std::string &op) {
    const runtime::Value left{context.eval(*node.child("left"))};
    const runtime::Value right{context.eval(*node.child("right"))};

    return compareNumeric(left, right, op);
}

} // namespace

NumericComparisonRuntimeFeature::NumericComparisonRuntimeFeature(std::string binaryNodeKind)
    : binaryNodeKind_{std::move(binaryNodeKind)} {
    if (binaryNodeKind_.empty()) {
        throw std::runtime_error("NumericComparisonRuntimeFeature::NumericComparisonRuntimeFeature: binary node kind cannot be empty");
    }
}

LanguageFeatureInfo NumericComparisonRuntimeFeature::info() const {
    return {"language.runtime.numeric-comparison", "0.2.0", "Runtime numeric comparisons with int/float promotion", {"runtime.operator.comparison"}, {"expression.binary", "runtime.value.integer"}, {"language.expression.binary"}, {}, {}};
}

void NumericComparisonRuntimeFeature::install(LanguageOptionsController &language) const {
    static_cast<void>(binaryNodeKind_);
    const std::array<std::string, 6> operators{"==", "!=", "<=", ">=", "<", ">"};

    for (const std::string &op : operators) {
        language.binaryOperator(op, [op](const ast::Node &node, runtime::RuntimeContext &context) {
            return evaluateBinary(node, context, op);
        });
    }
}

} // namespace novac::language::features
