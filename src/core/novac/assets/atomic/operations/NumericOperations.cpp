#include "novac/assets/atomic/operations/NumericOperations.hpp"

#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/assets/atomic/operations/IntegerArithmetic.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace novac::assets::atomic::operations {

namespace {

/**
 * @brief Checks the condition represented by `isInteger`.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
bool isInteger(const runtime::Value &value) {
    try {
        static_cast<void>(value.asInt());
        return true;
    } catch (const std::runtime_error &) {
        return false;
    }
}

/**
 * @brief Implements the `operatorKey` operation.
 *
 * @param info Value supplied for `info`.
 * @return Value produced by the operation.
 */
std::string operatorKey(const OperationInfo &info) {
    if (!info.pattern.token.empty()) {
        return info.pattern.token;
    }

    return info.pattern.tokenKey;
}

} // namespace

/**
 * @brief Constructs a `NumericBinaryOperationAtomic` instance.
 *
 * @param id Value supplied for `id`.
 * @param description Value supplied for `description`.
 * @param pattern Value supplied for `pattern`.
 * @param precedence Value supplied for `precedence`.
 */
NumericBinaryOperationAtomic::NumericBinaryOperationAtomic(std::string id, std::string description, TokenPattern pattern, int precedence)
    : id_{std::move(id)}, description_{std::move(description)}, pattern_{std::move(pattern)}, precedence_{precedence} {
    if (id_.empty()) {
        throw std::runtime_error("NumericBinaryOperationAtomic::NumericBinaryOperationAtomic: id cannot be empty");
    }
}

/**
 * @brief Implements the `info` operation.
 *
 * @return Value produced by the operation.
 */
OperationInfo NumericBinaryOperationAtomic::info() const {
    return {id_, "0.1.0", description_, OperationArity::Binary, pattern_, precedence_, parser::Associativity::Left, {"operation.numeric", "operation.binary"}, {"expression.atom"}
    };
}

/**
 * @brief Installs the behavior provided by `install`.
 *
 * @param controller Value supplied for `controller`.
 */
void NumericBinaryOperationAtomic::install(AtomicController &controller) const {
    const OperationInfo operation{info()};
    const std::string domain{controller.expressionDomain()};
    const std::string binaryKind{controller.binaryNodeKind()};
    const std::string opId{operation.id};
    const std::string key{operatorKey(operation)};
    auto *const engine{&controller.engine()};

    controller.registerPattern(operation.pattern);
    controller.ensureBinaryExpressionNode();

    const auto builder{[binaryKind, opId, engine](parser::ParserContext &, ast::NodePtr left, const token::Token &, ast::NodePtr right) {
        ast::NodePtr node{engine->makeNode(binaryKind)};
        node->set(fields::Operation, opId);
        node->set(fields::Left, left);
        node->set(fields::Right, right);
        return node;
    }};

    controller.engine().infix(domain, key, operation.precedence, operation.associativity, builder);

    const Evaluator evaluator{makeEvaluator()};

    controller.engine().binaryOperator(operation.id, [evaluator](const ast::Node &node, runtime::RuntimeContext &context) {
        const runtime::Value left{context.eval(*node.child(fields::Left))};
        const runtime::Value right{context.eval(*node.child(fields::Right))};
        return evaluator(left, right);
    });
}

/**
 * @brief Constructs a `AddOperationAtomic` instance.
 *
 * @param pattern Value supplied for `pattern`.
 */
AddOperationAtomic::AddOperationAtomic(TokenPattern pattern)
    : NumericBinaryOperationAtomic{"core.op.add", "Numeric addition operation", std::move(pattern), 10} {}

/**
 * @brief Creates a value through `makeEvaluator`.
 *
 * @return Value produced by the operation.
 */
auto AddOperationAtomic::makeEvaluator() const -> Evaluator {
    return [](const runtime::Value &left, const runtime::Value &right) {
        if (isInteger(left) && isInteger(right)) {
            return runtime::Value::integer(detail::checkedAdd(left.asInt(), right.asInt()));
        }

        return runtime::Value::floating(left.asFloat() + right.asFloat());
    };
}

/**
 * @brief Constructs a `SubtractOperationAtomic` instance.
 *
 * @param pattern Value supplied for `pattern`.
 */
SubtractOperationAtomic::SubtractOperationAtomic(TokenPattern pattern)
    : NumericBinaryOperationAtomic{"core.op.sub", "Numeric subtraction operation", std::move(pattern), 10} {}

/**
 * @brief Creates a value through `makeEvaluator`.
 *
 * @return Value produced by the operation.
 */
auto SubtractOperationAtomic::makeEvaluator() const -> Evaluator {
    return [](const runtime::Value &left, const runtime::Value &right) {
        if (isInteger(left) && isInteger(right)) {
            return runtime::Value::integer(detail::checkedSubtract(left.asInt(), right.asInt()));
        }

        return runtime::Value::floating(left.asFloat() - right.asFloat());
    };
}

/**
 * @brief Constructs a `MultiplyOperationAtomic` instance.
 *
 * @param pattern Value supplied for `pattern`.
 */
MultiplyOperationAtomic::MultiplyOperationAtomic(TokenPattern pattern)
    : NumericBinaryOperationAtomic{"core.op.mul", "Numeric multiplication operation", std::move(pattern), 20} {}

/**
 * @brief Creates a value through `makeEvaluator`.
 *
 * @return Value produced by the operation.
 */
auto MultiplyOperationAtomic::makeEvaluator() const -> Evaluator {
    return [](const runtime::Value &left, const runtime::Value &right) {
        if (isInteger(left) && isInteger(right)) {
            return runtime::Value::integer(detail::checkedMultiply(left.asInt(), right.asInt()));
        }

        return runtime::Value::floating(left.asFloat() * right.asFloat());
    };
}

/**
 * @brief Constructs a `DivideOperationAtomic` instance.
 *
 * @param pattern Value supplied for `pattern`.
 */
DivideOperationAtomic::DivideOperationAtomic(TokenPattern pattern)
    : NumericBinaryOperationAtomic{"core.op.div", "Numeric division operation", std::move(pattern), 20} {}

/**
 * @brief Creates a value through `makeEvaluator`.
 *
 * @return Value produced by the operation.
 */
auto DivideOperationAtomic::makeEvaluator() const -> Evaluator {
    return [](const runtime::Value &left, const runtime::Value &right) {
        if (isInteger(left) && isInteger(right)) {
            return runtime::Value::integer(
                detail::checkedDivide(left.asInt(), right.asInt()));
        }

        const double rhs{right.asFloat()};

        if (rhs == 0.0) {
            throw std::runtime_error("DivideOperationAtomic::evaluate: division by zero");
        }

        return runtime::Value::floating(left.asFloat() / rhs);
    };
}

/**
 * @brief Constructs a `ModuloOperationAtomic` instance.
 *
 * @param pattern Value supplied for `pattern`.
 */
ModuloOperationAtomic::ModuloOperationAtomic(TokenPattern pattern)
    : NumericBinaryOperationAtomic{"core.op.mod", "Numeric modulo operation", std::move(pattern), 20} {}

/**
 * @brief Creates a value through `makeEvaluator`.
 *
 * @return Value produced by the operation.
 */
auto ModuloOperationAtomic::makeEvaluator() const -> Evaluator {
    return [](const runtime::Value &left, const runtime::Value &right) {
        if (isInteger(left) && isInteger(right)) {
            return runtime::Value::integer(
                detail::checkedModulo(left.asInt(), right.asInt()));
        }

        const double rhs{right.asFloat()};

        if (rhs == 0.0) {
            throw std::runtime_error("ModuloOperationAtomic::evaluate: modulo by zero");
        }

        return runtime::Value::floating(std::fmod(left.asFloat(), rhs));
    };
}

} // namespace novac::assets::atomic::operations
