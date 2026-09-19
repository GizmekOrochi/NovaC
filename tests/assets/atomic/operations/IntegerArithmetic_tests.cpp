#include "../../../tester.hpp"
#include "novac/assets/atomic/operations/IntegerArithmetic.hpp"

#include <limits>
#include <stdexcept>

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

namespace arithmetic = novac::assets::atomic::operations::detail;

constexpr int minInt{std::numeric_limits<int>::min()};
constexpr int maxInt{std::numeric_limits<int>::max()};

TEST(IntegerArithmetic, AdditionAcceptsBoundaryResults) {
    CHECK(arithmetic::checkedAdd(maxInt - 1, 1) == maxInt);
    CHECK(arithmetic::checkedAdd(minInt + 1, -1) == minInt);
}

TEST(IntegerArithmetic, AdditionRejectsOverflow) {
    CHECK(throwsRuntimeError([]() { arithmetic::checkedAdd(maxInt, 1); }));
    CHECK(throwsRuntimeError([]() { arithmetic::checkedAdd(minInt, -1); }));
}

TEST(IntegerArithmetic, SubtractionAcceptsBoundaryResults) {
    CHECK(arithmetic::checkedSubtract(maxInt, 0) == maxInt);
    CHECK(arithmetic::checkedSubtract(minInt, 0) == minInt);
}

TEST(IntegerArithmetic, SubtractionRejectsOverflow) {
    CHECK(throwsRuntimeError([]() { arithmetic::checkedSubtract(maxInt, -1); }));
    CHECK(throwsRuntimeError([]() { arithmetic::checkedSubtract(minInt, 1); }));
}

TEST(IntegerArithmetic, MultiplicationAcceptsBoundaryResults) {
    CHECK(arithmetic::checkedMultiply(maxInt, 1) == maxInt);
    CHECK(arithmetic::checkedMultiply(minInt, 1) == minInt);
    CHECK(arithmetic::checkedMultiply(0, minInt) == 0);
}

TEST(IntegerArithmetic, MultiplicationRejectsOverflow) {
    CHECK(throwsRuntimeError([]() { arithmetic::checkedMultiply(maxInt, 2); }));
    CHECK(throwsRuntimeError([]() { arithmetic::checkedMultiply(minInt, -1); }));
    CHECK(throwsRuntimeError([]() { arithmetic::checkedMultiply(minInt, 2); }));
}

TEST(IntegerArithmetic, DivisionRejectsZeroAndOverflow) {
    CHECK(arithmetic::checkedDivide(minInt, 1) == minInt);
    CHECK(throwsRuntimeError([]() { arithmetic::checkedDivide(1, 0); }));
    CHECK(throwsRuntimeError([]() { arithmetic::checkedDivide(minInt, -1); }));
}

TEST(IntegerArithmetic, ModuloDefinesMinByNegativeOneAsZero) {
    CHECK(arithmetic::checkedModulo(minInt, -1) == 0);
    CHECK(arithmetic::checkedModulo(7, 3) == 1);
    CHECK(throwsRuntimeError([]() { arithmetic::checkedModulo(1, 0); }));
}

TEST(IntegerArithmetic, NegationRejectsMinValue) {
    CHECK(arithmetic::checkedNegate(maxInt) == -maxInt);
    CHECK(throwsRuntimeError([]() { arithmetic::checkedNegate(minInt); }));
}

} // namespace
