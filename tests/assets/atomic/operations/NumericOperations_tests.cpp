#include "../../../tester.hpp"
#include "novac/assets/atomic/operations/NumericOperations.hpp"
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

namespace test {
    class TestNumericBinaryOperation : public novac::assets::atomic::operations::NumericBinaryOperationAtomic {
    public:
        TestNumericBinaryOperation(std::string id, std::string description, novac::assets::atomic::TokenPattern pattern, int precedence)
            : NumericBinaryOperationAtomic(std::move(id), std::move(description), std::move(pattern), precedence) {}

        novac::runtime::Value evaluate(const novac::runtime::Value &, const novac::runtime::Value &) const override {
            return novac::runtime::Value::integer(0);
        }
    };
} // namespace test

using novac::assets::atomic::operations::NumericBinaryOperationAtomic;
using novac::assets::atomic::operations::AddOperationAtomic;
using novac::assets::atomic::operations::SubtractOperationAtomic;
using novac::assets::atomic::operations::MultiplyOperationAtomic;
using novac::assets::atomic::operations::DivideOperationAtomic;
using novac::assets::atomic::operations::ModuloOperationAtomic;
using novac::assets::atomic::OperationInfo;
using novac::assets::atomic::TokenPattern;
using novac::parser::Associativity;

TEST(NumericBinaryOperationAtomic, EmptyIdThrows) {
    CHECK(throwsRuntimeError([]() { test::TestNumericBinaryOperation op("", "desc", TokenPattern::text("+"), 10); }));
}

TEST(NumericBinaryOperationAtomic, CustomConstruction) {
    test::TestNumericBinaryOperation op("custom.op", "Custom op", TokenPattern::text("^"), 15);
    OperationInfo info = op.info();
    CHECK(info.id == "custom.op");
    CHECK(info.description == "Custom op");
    CHECK(info.pattern.token == "^");
    CHECK(info.precedence == 15);
    CHECK(info.arity == novac::assets::atomic::OperationArity::Binary);
    CHECK(info.associativity == Associativity::Left);
    CHECK(info.capabilities.size() == 2);
    CHECK(info.capabilities[0] == "operation.numeric");
    CHECK(info.capabilities[1] == "operation.binary");
}

TEST(AddOperationAtomic, DefaultConstruction) {
    AddOperationAtomic op;
    OperationInfo info = op.info();
    CHECK(info.id == "core.op.add");
    CHECK(info.description == "Numeric addition operation");
    CHECK(info.pattern.token == "+");
    CHECK(info.precedence == 10);
}

TEST(AddOperationAtomic, CustomPattern) {
    AddOperationAtomic op(TokenPattern::text("plus"));
    OperationInfo info = op.info();
    CHECK(info.pattern.token == "plus");
}

TEST(SubtractOperationAtomic, DefaultConstruction) {
    SubtractOperationAtomic op;
    OperationInfo info = op.info();
    CHECK(info.id == "core.op.sub");
    CHECK(info.pattern.token == "-");
    CHECK(info.precedence == 10);
}

TEST(MultiplyOperationAtomic, DefaultConstruction) {
    MultiplyOperationAtomic op;
    OperationInfo info = op.info();
    CHECK(info.id == "core.op.mul");
    CHECK(info.pattern.token == "*");
    CHECK(info.precedence == 20);
}

TEST(DivideOperationAtomic, DefaultConstruction) {
    DivideOperationAtomic op;
    OperationInfo info = op.info();
    CHECK(info.id == "core.op.div");
    CHECK(info.pattern.token == "/");
    CHECK(info.precedence == 20);
}

TEST(ModuloOperationAtomic, DefaultConstruction) {
    ModuloOperationAtomic op;
    OperationInfo info = op.info();
    CHECK(info.id == "core.op.mod");
    CHECK(info.pattern.token == "%");
    CHECK(info.precedence == 20);
}

} // namespace