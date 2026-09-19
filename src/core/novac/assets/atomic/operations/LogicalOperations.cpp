#include "novac/assets/atomic/operations/LogicalOperations.hpp"

#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/assets/atomic/operations/IntegerArithmetic.hpp"

#include <stdexcept>
#include <utility>

namespace novac::assets::atomic::operations {

namespace {

bool isInteger(const runtime::Value &value) {
    try {
        static_cast<void>(value.asInt());
        return true;
    } catch (const std::runtime_error &) {
        return false;
    }
}

std::string operatorKey(const OperationInfo &info) {
    if (!info.pattern.token.empty()) {
        return info.pattern.token;
    }

    return info.pattern.tokenKey;
}

void installBinaryLogical(
    AtomicController &controller,
    const OperationInfo &operation,
    runtime::BinaryHandler handler) {
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
    controller.engine().binaryOperator(operation.id, std::move(handler));
}

} // namespace

LogicalAndOperationAtomic::LogicalAndOperationAtomic(TokenPattern pattern)
 : pattern_{std::move(pattern)} {}

OperationInfo LogicalAndOperationAtomic::info() const {
    return {"core.op.logical.and", "0.1.0", "Logical AND operation", OperationArity::Binary, pattern_, 4, parser::Associativity::Left, {"operation.logical", "operation.binary"}, {"expression.atom"}};
}

void LogicalAndOperationAtomic::install(AtomicController &controller) const {
    installBinaryLogical(controller, info(), [](const ast::Node &node, runtime::RuntimeContext &context) {
        const bool left{context.eval(*node.child("left")).truthy()};

        if (!left) {
            return runtime::Value::boolean(false);
        }

        return runtime::Value::boolean(context.eval(*node.child("right")).truthy());
    });
}

LogicalOrOperationAtomic::LogicalOrOperationAtomic(TokenPattern pattern)
    : pattern_{std::move(pattern)} {}

OperationInfo LogicalOrOperationAtomic::info() const {
    return {"core.op.logical.or", "0.1.0", "Logical OR operation", OperationArity::Binary, pattern_, 3, parser::Associativity::Left, {"operation.logical", "operation.binary"}, {"expression.atom"}};
}

void LogicalOrOperationAtomic::install(AtomicController &controller) const {
    installBinaryLogical(controller, info(), [](const ast::Node &node, runtime::RuntimeContext &context) {
        const bool left{context.eval(*node.child("left")).truthy()};

        if (left) {
            return runtime::Value::boolean(true);
        }

        return runtime::Value::boolean(context.eval(*node.child("right")).truthy());
    });
}

LogicalNotOperationAtomic::LogicalNotOperationAtomic(TokenPattern pattern)
    : pattern_{std::move(pattern)} {}

OperationInfo LogicalNotOperationAtomic::info() const {
    return {"core.op.logical.not", "0.1.0", "Logical NOT operation", OperationArity::Unary, pattern_, 30, parser::Associativity::Right, {"operation.logical", "operation.unary"}, {"expression.atom"}};
}

void LogicalNotOperationAtomic::install(AtomicController &controller) const {
    const OperationInfo operation{info()};
    const std::string domain{controller.expressionDomain()};
    const std::string unaryKind{controller.unaryNodeKind()};
    const std::string opId{operation.id};
    const std::string key{operatorKey(operation)};

    controller.registerPattern(operation.pattern);
    controller.ensureUnaryExpressionNode();

    controller.engine().prefix(domain, key, [domain, unaryKind, opId, &controller](parser::ParserContext &context) {
        context.advance();
        ast::NodePtr operand{context.parse(domain, 30)};
        ast::NodePtr node{controller.engine().makeNode(unaryKind)};
        node->set("op", opId);
        node->set("expr", operand);
        return node;
    });

    controller.registerUnaryOperation(operation.id, [](const ast::Node &node, runtime::RuntimeContext &context) {
        return runtime::Value::boolean(!context.eval(*node.child("expr")).truthy());
    });
}


NumericNegateOperationAtomic::NumericNegateOperationAtomic(TokenPattern pattern)
    : pattern_{std::move(pattern)} {}

OperationInfo NumericNegateOperationAtomic::info() const {
    return {"core.op.neg", "0.1.0", "Numeric negation operation", OperationArity::Unary, pattern_, 30, parser::Associativity::Right, {"operation.numeric", "operation.unary"}, {"expression.atom"}};
}

void NumericNegateOperationAtomic::install(AtomicController &controller) const {
    const OperationInfo operation{info()};
    const std::string domain{controller.expressionDomain()};
    const std::string unaryKind{controller.unaryNodeKind()};
    const std::string opId{operation.id};
    const std::string key{operatorKey(operation)};

    controller.registerPattern(operation.pattern);
    controller.ensureUnaryExpressionNode();

    controller.engine().prefix(domain, key, [domain, unaryKind, opId, &controller](parser::ParserContext &context) {
        context.advance();
        ast::NodePtr operand{context.parse(domain, 30)};
        ast::NodePtr node{controller.engine().makeNode(unaryKind)};
        node->set("op", opId);
        node->set("expr", operand);
        return node;
    });

    controller.registerUnaryOperation(operation.id, [](const ast::Node &node, runtime::RuntimeContext &context) {
        const runtime::Value value{context.eval(*node.child("expr"))};

        if (isInteger(value)) {
            return runtime::Value::integer(detail::checkedNegate(value.asInt()));
        }

        return runtime::Value::floating(-value.asFloat());
    });
}

} // namespace novac::assets::atomic::operations
