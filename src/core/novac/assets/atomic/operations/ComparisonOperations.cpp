#include "novac/assets/atomic/operations/ComparisonOperations.hpp"

#include "novac/assets/atomic/AtomicController.hpp"

#include <stdexcept>
#include <utility>

namespace novac::assets::atomic::operations {

namespace {

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
 * @brief Constructs a `NumericComparisonOperationAtomic` instance.
 *
 * @param id Value supplied for `id`.
 * @param description Value supplied for `description`.
 * @param pattern Value supplied for `pattern`.
 */
NumericComparisonOperationAtomic::NumericComparisonOperationAtomic(std::string id, std::string description, TokenPattern pattern)
    : id_{std::move(id)}, description_{std::move(description)}, pattern_{std::move(pattern)} {
    if (id_.empty()) {
        throw std::runtime_error("NumericComparisonOperationAtomic::NumericComparisonOperationAtomic: id cannot be empty");
    }
}

/**
 * @brief Implements the `info` operation.
 *
 * @return Value produced by the operation.
 */
OperationInfo NumericComparisonOperationAtomic::info() const {
    return {id_, "0.1.0", description_, OperationArity::Binary, pattern_, 7, parser::Associativity::None, {"operation.comparison", "operation.binary"}, {"expression.atom"}};
}

/**
 * @brief Installs the behavior provided by `install`.
 *
 * @param controller Value supplied for `controller`.
 */
void NumericComparisonOperationAtomic::install(AtomicController &controller) const {
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

    const Comparator comparator{makeComparator()};

    controller.engine().binaryOperator(operation.id, [comparator](const ast::Node &node, runtime::RuntimeContext &context) {
        const double left{context.eval(*node.child(fields::Left)).asFloat()};
        const double right{context.eval(*node.child(fields::Right)).asFloat()};
        return runtime::Value::boolean(comparator(left, right));
    });
}

/**
 * @brief Constructs a `EqualOperationAtomic` instance.
 *
 * @param pattern Value supplied for `pattern`.
 */
EqualOperationAtomic::EqualOperationAtomic(TokenPattern pattern)
    : NumericComparisonOperationAtomic{"core.op.eq", "Numeric equality operation", std::move(pattern)} {}

/**
 * @brief Creates a value through `makeComparator`.
 *
 * @return Value produced by the operation.
 */
auto EqualOperationAtomic::makeComparator() const -> Comparator {
    return [](double left, double right) {
        return left == right;
    };
}

/**
 * @brief Constructs a `NotEqualOperationAtomic` instance.
 *
 * @param pattern Value supplied for `pattern`.
 */
NotEqualOperationAtomic::NotEqualOperationAtomic(TokenPattern pattern)
    : NumericComparisonOperationAtomic{"core.op.neq", "Numeric inequality operation", std::move(pattern)} {}

/**
 * @brief Creates a value through `makeComparator`.
 *
 * @return Value produced by the operation.
 */
auto NotEqualOperationAtomic::makeComparator() const -> Comparator {
    return [](double left, double right) {
        return left != right;
    };
}

/**
 * @brief Constructs a `LessOperationAtomic` instance.
 *
 * @param pattern Value supplied for `pattern`.
 */
LessOperationAtomic::LessOperationAtomic(TokenPattern pattern)
    : NumericComparisonOperationAtomic{"core.op.lt", "Numeric less-than operation", std::move(pattern)} {}

/**
 * @brief Creates a value through `makeComparator`.
 *
 * @return Value produced by the operation.
 */
auto LessOperationAtomic::makeComparator() const -> Comparator {
    return [](double left, double right) {
        return left < right;
    };
}

/**
 * @brief Constructs a `LessEqualOperationAtomic` instance.
 *
 * @param pattern Value supplied for `pattern`.
 */
LessEqualOperationAtomic::LessEqualOperationAtomic(TokenPattern pattern)
    : NumericComparisonOperationAtomic{"core.op.lte", "Numeric less-or-equal operation", std::move(pattern)} {}

/**
 * @brief Creates a value through `makeComparator`.
 *
 * @return Value produced by the operation.
 */
auto LessEqualOperationAtomic::makeComparator() const -> Comparator {
    return [](double left, double right) {
        return left <= right;
    };
}

/**
 * @brief Constructs a `GreaterOperationAtomic` instance.
 *
 * @param pattern Value supplied for `pattern`.
 */
GreaterOperationAtomic::GreaterOperationAtomic(TokenPattern pattern)
    : NumericComparisonOperationAtomic{"core.op.gt", "Numeric greater-than operation", std::move(pattern)} {}

/**
 * @brief Creates a value through `makeComparator`.
 *
 * @return Value produced by the operation.
 */
auto GreaterOperationAtomic::makeComparator() const -> Comparator {
    return [](double left, double right) {
        return left > right;
    };
}

/**
 * @brief Constructs a `GreaterEqualOperationAtomic` instance.
 *
 * @param pattern Value supplied for `pattern`.
 */
GreaterEqualOperationAtomic::GreaterEqualOperationAtomic(TokenPattern pattern)
    : NumericComparisonOperationAtomic{"core.op.gte", "Numeric greater-or-equal operation", std::move(pattern)} {}

/**
 * @brief Creates a value through `makeComparator`.
 *
 * @return Value produced by the operation.
 */
auto GreaterEqualOperationAtomic::makeComparator() const -> Comparator {
    return [](double left, double right) {
        return left >= right;
    };
}

} // namespace novac::assets::atomic::operations
