#include "../../../tester.hpp"
#include "novac/assets/atomic/operations/ComparisonOperations.hpp"
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
    class TestComparisonOperation : public novac::assets::atomic::operations::NumericComparisonOperationAtomic {
    public:
        TestComparisonOperation(std::string id, std::string description, novac::assets::atomic::TokenPattern pattern)
            : NumericComparisonOperationAtomic(std::move(id), std::move(description), std::move(pattern)) {}

        bool compare(double, double) const override {
            return false;
        }
    };
} // namespace test

using novac::assets::atomic::operations::NumericComparisonOperationAtomic;
using novac::assets::atomic::operations::EqualOperationAtomic;
using novac::assets::atomic::operations::NotEqualOperationAtomic;
using novac::assets::atomic::operations::LessOperationAtomic;
using novac::assets::atomic::operations::LessEqualOperationAtomic;
using novac::assets::atomic::operations::GreaterOperationAtomic;
using novac::assets::atomic::operations::GreaterEqualOperationAtomic;
using novac::assets::atomic::OperationInfo;
using novac::assets::atomic::TokenPattern;
using novac::parser::Associativity;

TEST(NumericComparisonOperationAtomic, EmptyIdThrows) {
    CHECK(throwsRuntimeError([]() { test::TestComparisonOperation op("", "desc", TokenPattern::text("==")); }));
}

TEST(NumericComparisonOperationAtomic, CustomConstruction) {
    test::TestComparisonOperation op("custom.cmp", "Custom comparison", TokenPattern::text("<>"));
    OperationInfo info = op.info();
    CHECK(info.id == "custom.cmp");
    CHECK(info.description == "Custom comparison");
    CHECK(info.pattern.token == "<>");
    CHECK(info.precedence == 7);
    CHECK(info.associativity == Associativity::None);
    CHECK(info.arity == novac::assets::atomic::OperationArity::Binary);
    CHECK(info.capabilities[0] == "operation.comparison");
    CHECK(info.capabilities[1] == "operation.binary");
}

TEST(EqualOperationAtomic, DefaultConstruction) {
    EqualOperationAtomic op;
    OperationInfo info = op.info();
    CHECK(info.id == "core.op.eq");
    CHECK(info.description == "Numeric equality operation");
    CHECK(info.pattern.token == "==");
    CHECK(info.precedence == 7);
}

TEST(NotEqualOperationAtomic, DefaultConstruction) {
    NotEqualOperationAtomic op;
    OperationInfo info = op.info();
    CHECK(info.id == "core.op.neq");
    CHECK(info.pattern.token == "!=");
}

TEST(LessOperationAtomic, DefaultConstruction) {
    LessOperationAtomic op;
    OperationInfo info = op.info();
    CHECK(info.id == "core.op.lt");
    CHECK(info.pattern.token == "<");
}

TEST(LessEqualOperationAtomic, DefaultConstruction) {
    LessEqualOperationAtomic op;
    OperationInfo info = op.info();
    CHECK(info.id == "core.op.lte");
    CHECK(info.pattern.token == "<=");
}

TEST(GreaterOperationAtomic, DefaultConstruction) {
    GreaterOperationAtomic op;
    OperationInfo info = op.info();
    CHECK(info.id == "core.op.gt");
    CHECK(info.pattern.token == ">");
}

TEST(GreaterEqualOperationAtomic, DefaultConstruction) {
    GreaterEqualOperationAtomic op;
    OperationInfo info = op.info();
    CHECK(info.id == "core.op.gte");
    CHECK(info.pattern.token == ">=");
}

TEST(ComparisonOperations, CustomPattern) {
    EqualOperationAtomic op(TokenPattern::text("="));
    OperationInfo info = op.info();
    CHECK(info.pattern.token == "=");
}

} // namespace