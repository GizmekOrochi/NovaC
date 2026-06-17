#include "../../tester.hpp"
#include "novac/engine/transformation/IR.hpp"

#include <stdexcept>
#include <vector>

namespace {

using novac::ir::MIRLoweringContext;
using novac::ir::Operand;
using novac::ir::ValueId;

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

} // namespace

TEST(MIRLoweringContext, StartsWithoutBindings) {
    MIRLoweringContext context{};

    CHECK(!context.has(ValueId{1}));
}

TEST(MIRLoweringContext, BindsAndResolvesValues) {
    MIRLoweringContext context{};

    context.bind(ValueId{1}, ValueId{10});

    CHECK(context.has(ValueId{1}));
    CHECK(context.resolve(ValueId{1}).value == 10);
}

TEST(MIRLoweringContext, ReplacesExistingBinding) {
    MIRLoweringContext context{};

    context.bind(ValueId{1}, ValueId{10});
    context.bind(ValueId{1}, ValueId{20});

    CHECK(context.resolve(ValueId{1}).value == 20);
}

TEST(MIRLoweringContext, RejectsUnknownResolution) {
    MIRLoweringContext context{};

    CHECK(throwsRuntimeError([&]() { static_cast<void>(context.resolve(ValueId{999})); }));
}

TEST(MIRLoweringContext, RemapsValueOperand) {
    MIRLoweringContext context{};
    context.bind(ValueId{1}, ValueId{100});

    Operand operand{context.remapOperand(Operand::fromValue(ValueId{1}))};

    CHECK(operand.isValue());
    CHECK(operand.asValue().value == 100);
}

TEST(MIRLoweringContext, LeavesNonValueOperandsUnchanged) {
    MIRLoweringContext context{};

    Operand literal{context.remapOperand(Operand::fromLiteral(42))};
    Operand symbol{context.remapOperand(Operand::fromSymbol("x"))};

    CHECK(literal.toString() == "42");
    CHECK(symbol.toString() == "x");
}

TEST(MIRLoweringContext, RejectsUnmappedValueOperand) {
    MIRLoweringContext context{};

    CHECK(throwsRuntimeError([&]() { static_cast<void>(context.remapOperand(Operand::fromValue(ValueId{1}))); }));
}

TEST(MIRLoweringContext, RemapsOperandList) {
    MIRLoweringContext context{};
    context.bind(ValueId{1}, ValueId{10});
    context.bind(ValueId{2}, ValueId{20});

    std::vector<Operand> remapped{context.remapOperands({
        Operand::fromValue(ValueId{1}),
        Operand::fromLiteral(5),
        Operand::fromValue(ValueId{2})
    })};

    CHECK(remapped.size() == 3);
    CHECK(remapped[0].asValue().value == 10);
    CHECK(remapped[1].toString() == "5");
    CHECK(remapped[2].asValue().value == 20);
}
