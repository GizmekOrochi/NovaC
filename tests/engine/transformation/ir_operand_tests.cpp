#include "../../tester.hpp"
#include "novac/engine/transformation/IR.hpp"

#include <stdexcept>
#include <string>
#include <variant>

namespace {

using novac::ir::Literal;
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

TEST(Operand, CreatesValueOperand) {
    Operand operand{Operand::fromValue(ValueId{7})};

    CHECK(operand.isValue());
    CHECK(operand.asValue().value == 7);
    CHECK(operand.toString() == "v7");
}

TEST(Operand, CreatesLiteralOperandFromLiteral) {
    Operand operand{Operand::fromLiteral(Literal::integer(42))};

    CHECK(!operand.isValue());
    CHECK(operand.toString() == "42");
}

TEST(Operand, CreatesLiteralOperandsFromPrimitives) {
    CHECK(Operand::fromLiteral(42).toString() == "42");
    CHECK(Operand::fromLiteral(3.5).toString().find("3.500000") != std::string::npos);
    CHECK(Operand::fromLiteral(true).toString() == "true");
    CHECK(Operand::fromLiteral(std::string{"hello"}).toString() == "hello");
}

TEST(Operand, CreatesSymbolOperand) {
    Operand operand{Operand::fromSymbol("x")};

    CHECK(!operand.isValue());
    CHECK(operand.toString() == "x");
    CHECK(std::holds_alternative<std::string>(operand.data()));
}

TEST(Operand, RejectsAsValueWhenNotValue) {
    CHECK(throwsRuntimeError([]() { static_cast<void>(Operand::fromLiteral(1).asValue()); }));
    CHECK(throwsRuntimeError([]() { static_cast<void>(Operand::fromSymbol("x").asValue()); }));
}

TEST(Operand, RawVariantConstruction) {
    Operand operand{Operand::Data{ValueId{9}}};

    CHECK(operand.isValue());
    CHECK(operand.asValue().value == 9);
}
