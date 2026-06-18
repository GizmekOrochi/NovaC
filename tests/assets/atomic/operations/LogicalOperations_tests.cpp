#include "../../../tester.hpp"
#include "novac/assets/atomic/operations/LogicalOperations.hpp"
#include "novac/assets/atomic/AtomicPattern.hpp"

namespace {

template <typename Fn>
bool throwsRuntimeError(Fn &&fn) {
    try {
        fn();
    } catch (const std::runtime_error &) {
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

using novac::assets::atomic::operations::LogicalAndOperationAtomic;
using novac::assets::atomic::operations::LogicalOrOperationAtomic;
using novac::assets::atomic::operations::LogicalNotOperationAtomic;
using novac::assets::atomic::operations::NumericNegateOperationAtomic;
using novac::assets::atomic::OperationInfo;
using novac::assets::atomic::TokenPattern;
using novac::parser::Associativity;

TEST(LogicalAndOperationAtomic, DefaultConstruction) {
    LogicalAndOperationAtomic op;
    OperationInfo info = op.info();
    CHECK(info.id == "core.op.logical.and");
    CHECK(info.description == "Logical AND operation");
    CHECK(info.pattern.token == "&&");
    CHECK(info.precedence == 4);
    CHECK(info.arity == novac::assets::atomic::OperationArity::Binary);
    CHECK(info.associativity == Associativity::Left);
    CHECK(info.capabilities[0] == "operation.logical");
    CHECK(info.capabilities[1] == "operation.binary");
}

TEST(LogicalAndOperationAtomic, CustomPattern) {
    LogicalAndOperationAtomic op(TokenPattern::text("AND"));
    OperationInfo info = op.info();
    CHECK(info.pattern.token == "AND");
}

TEST(LogicalOrOperationAtomic, DefaultConstruction) {
    LogicalOrOperationAtomic op;
    OperationInfo info = op.info();
    CHECK(info.id == "core.op.logical.or");
    CHECK(info.description == "Logical OR operation");
    CHECK(info.pattern.token == "||");
    CHECK(info.precedence == 3);
}

TEST(LogicalNotOperationAtomic, DefaultConstruction) {
    LogicalNotOperationAtomic op;
    OperationInfo info = op.info();
    CHECK(info.id == "core.op.logical.not");
    CHECK(info.description == "Logical NOT operation");
    CHECK(info.pattern.token == "!");
    CHECK(info.precedence == 30);
    CHECK(info.arity == novac::assets::atomic::OperationArity::Unary);
    CHECK(info.associativity == Associativity::Right);
}

TEST(NumericNegateOperationAtomic, DefaultConstruction) {
    NumericNegateOperationAtomic op;
    OperationInfo info = op.info();
    CHECK(info.id == "core.op.neg");
    CHECK(info.description == "Numeric negation operation");
    CHECK(info.pattern.token == "-");
    CHECK(info.precedence == 30);
    CHECK(info.arity == novac::assets::atomic::OperationArity::Unary);
    CHECK(info.capabilities[0] == "operation.numeric");
    CHECK(info.capabilities[1] == "operation.unary");
}

TEST(NumericNegateOperationAtomic, CustomPattern) {
    NumericNegateOperationAtomic op(TokenPattern::text("neg"));
    OperationInfo info = op.info();
    CHECK(info.pattern.token == "neg");
}

} // namespace