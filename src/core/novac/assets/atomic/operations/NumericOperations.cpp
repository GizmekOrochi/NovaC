#include "novac/assets/atomic/operations/NumericOperations.hpp"

#include "novac/assets/atomic/AtomicController.hpp"

#include <cmath>
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

} // namespace

NumericBinaryOperationAtomic::NumericBinaryOperationAtomic(std::string id, std::string description, TokenPattern pattern, int precedence)
    : id_{std::move(id)}, description_{std::move(description)}, pattern_{std::move(pattern)}, precedence_{precedence} {
    if (id_.empty()) {
        throw std::runtime_error("NumericBinaryOperationAtomic::NumericBinaryOperationAtomic: id cannot be empty");
    }
}

OperationInfo NumericBinaryOperationAtomic::info() const {
    return {id_, "0.1.0", description_, OperationArity::Binary, pattern_, precedence_, parser::Associativity::Left, {"operation.numeric", "operation.binary"}, {"expression.atom"}
    };
}

void NumericBinaryOperationAtomic::install(AtomicController &controller) const {
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

    controller.engine().binaryOperator(operation.id, [this](const ast::Node &node, runtime::RuntimeContext &context) {
        const runtime::Value left{context.eval(*node.child("left"))};
        const runtime::Value right{context.eval(*node.child("right"))};
        return evaluate(left, right);
    });
}

AddOperationAtomic::AddOperationAtomic(TokenPattern pattern)
    : NumericBinaryOperationAtomic{"core.op.add", "Numeric addition operation", std::move(pattern), 10} {}

runtime::Value AddOperationAtomic::evaluate(const runtime::Value &left, const runtime::Value &right) const {
    if (isInteger(left) && isInteger(right)) {
        return runtime::Value::integer(left.asInt() + right.asInt());
    }

    return runtime::Value::floating(left.asFloat() + right.asFloat());
}

SubtractOperationAtomic::SubtractOperationAtomic(TokenPattern pattern)
    : NumericBinaryOperationAtomic{"core.op.sub", "Numeric subtraction operation", std::move(pattern), 10} {}

runtime::Value SubtractOperationAtomic::evaluate(const runtime::Value &left, const runtime::Value &right) const {
    if (isInteger(left) && isInteger(right)) {
        return runtime::Value::integer(left.asInt() - right.asInt());
    }

    return runtime::Value::floating(left.asFloat() - right.asFloat());
}

MultiplyOperationAtomic::MultiplyOperationAtomic(TokenPattern pattern)
    : NumericBinaryOperationAtomic{"core.op.mul", "Numeric multiplication operation", std::move(pattern), 20} {}

runtime::Value MultiplyOperationAtomic::evaluate(const runtime::Value &left, const runtime::Value &right) const {
    if (isInteger(left) && isInteger(right)) {
        return runtime::Value::integer(left.asInt() * right.asInt());
    }

    return runtime::Value::floating(left.asFloat() * right.asFloat());
}

DivideOperationAtomic::DivideOperationAtomic(TokenPattern pattern)
    : NumericBinaryOperationAtomic{"core.op.div", "Numeric division operation", std::move(pattern), 20} {}

runtime::Value DivideOperationAtomic::evaluate(const runtime::Value &left, const runtime::Value &right) const {
    if (isInteger(left) && isInteger(right)) {
        const int rhs{right.asInt()};

        if (rhs == 0) {
            throw std::runtime_error("DivideOperationAtomic::evaluate: division by zero");
        }

        return runtime::Value::integer(left.asInt() / rhs);
    }

    const double rhs{right.asFloat()};

    if (rhs == 0.0) {
        throw std::runtime_error("DivideOperationAtomic::evaluate: division by zero");
    }

    return runtime::Value::floating(left.asFloat() / rhs);
}

ModuloOperationAtomic::ModuloOperationAtomic(TokenPattern pattern)
    : NumericBinaryOperationAtomic{"core.op.mod", "Numeric modulo operation", std::move(pattern), 20} {}

runtime::Value ModuloOperationAtomic::evaluate(const runtime::Value &left, const runtime::Value &right) const {
    if (isInteger(left) && isInteger(right)) {
        const int rhs{right.asInt()};

        if (rhs == 0) {
            throw std::runtime_error("ModuloOperationAtomic::evaluate: modulo by zero");
        }

        return runtime::Value::integer(left.asInt() % rhs);
    }

    const double rhs{right.asFloat()};

    if (rhs == 0.0) {
        throw std::runtime_error("ModuloOperationAtomic::evaluate: modulo by zero");
    }

    return runtime::Value::floating(std::fmod(left.asFloat(), rhs));
}

} // namespace novac::assets::atomic::operations
