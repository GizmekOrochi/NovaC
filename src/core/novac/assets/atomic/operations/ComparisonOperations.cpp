#include "novac/assets/atomic/operations/ComparisonOperations.hpp"

#include "novac/assets/atomic/AtomicController.hpp"

#include <stdexcept>
#include <utility>

namespace novac::assets::atomic::operations {

namespace {

std::string operatorKey(const OperationInfo &info) {
    if (!info.pattern.token.empty()) {
        return info.pattern.token;
    }

    return info.pattern.tokenKey;
}

} // namespace

NumericComparisonOperationAtomic::NumericComparisonOperationAtomic(std::string id, std::string description, TokenPattern pattern)
    : id_{std::move(id)}, description_{std::move(description)}, pattern_{std::move(pattern)} {
    if (id_.empty()) {
        throw std::runtime_error("NumericComparisonOperationAtomic::NumericComparisonOperationAtomic: id cannot be empty");
    }
}

OperationInfo NumericComparisonOperationAtomic::info() const {
    return {id_, "0.1.0", description_, OperationArity::Binary, pattern_, 7, parser::Associativity::None, {"operation.comparison", "operation.binary"}, {"expression.atom"}};
}

void NumericComparisonOperationAtomic::install(AtomicController &controller) const {
    const OperationInfo operation{info()};
    const std::string domain{controller.expressionDomain()};
    const std::string binaryKind{controller.binaryNodeKind()};
    const std::string opId{operation.id};
    const std::string key{operatorKey(operation)};

    controller.registerPattern(operation.pattern);
    controller.ensureBinaryExpressionNode();

    const auto builder{[binaryKind, opId, &controller](parser::ParserContext &, ast::NodePtr left, const token::Token &, ast::NodePtr right) {
        ast::NodePtr node{controller.engine().makeNode(binaryKind)};
        node->set("op", opId);
        node->set("left", left);
        node->set("right", right);
        return node;
    }};

    controller.engine().infix(domain, key, operation.precedence, operation.associativity, builder);

    const Comparator comparator{makeComparator()};

    controller.engine().binaryOperator(operation.id, [comparator](const ast::Node &node, runtime::RuntimeContext &context) {
        const double left{context.eval(*node.child("left")).asFloat()};
        const double right{context.eval(*node.child("right")).asFloat()};
        return runtime::Value::boolean(comparator(left, right));
    });
}

EqualOperationAtomic::EqualOperationAtomic(TokenPattern pattern)
    : NumericComparisonOperationAtomic{"core.op.eq", "Numeric equality operation", std::move(pattern)} {}

auto EqualOperationAtomic::makeComparator() const -> Comparator {
    return [](double left, double right) {
        return left == right;
    };
}

NotEqualOperationAtomic::NotEqualOperationAtomic(TokenPattern pattern)
    : NumericComparisonOperationAtomic{"core.op.neq", "Numeric inequality operation", std::move(pattern)} {}

auto NotEqualOperationAtomic::makeComparator() const -> Comparator {
    return [](double left, double right) {
        return left != right;
    };
}

LessOperationAtomic::LessOperationAtomic(TokenPattern pattern)
    : NumericComparisonOperationAtomic{"core.op.lt", "Numeric less-than operation", std::move(pattern)} {}

auto LessOperationAtomic::makeComparator() const -> Comparator {
    return [](double left, double right) {
        return left < right;
    };
}

LessEqualOperationAtomic::LessEqualOperationAtomic(TokenPattern pattern)
    : NumericComparisonOperationAtomic{"core.op.lte", "Numeric less-or-equal operation", std::move(pattern)} {}

auto LessEqualOperationAtomic::makeComparator() const -> Comparator {
    return [](double left, double right) {
        return left <= right;
    };
}

GreaterOperationAtomic::GreaterOperationAtomic(TokenPattern pattern)
    : NumericComparisonOperationAtomic{"core.op.gt", "Numeric greater-than operation", std::move(pattern)} {}

auto GreaterOperationAtomic::makeComparator() const -> Comparator {
    return [](double left, double right) {
        return left > right;
    };
}

GreaterEqualOperationAtomic::GreaterEqualOperationAtomic(TokenPattern pattern)
    : NumericComparisonOperationAtomic{"core.op.gte", "Numeric greater-or-equal operation", std::move(pattern)} {}

auto GreaterEqualOperationAtomic::makeComparator() const -> Comparator {
    return [](double left, double right) {
        return left >= right;
    };
}

} // namespace novac::assets::atomic::operations
