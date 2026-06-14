#include <iostream>
#include <string>

#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/engine/EngineController.hpp"

namespace {

void printExpression(const novac::controllers::EngineController &engine, const std::string &source) {
    const novac::ast::NodePtr root{engine.parse(source)};

    engine.validate(*root);

    const novac::runtime::Value value{engine.eval(*root)};

    std::cout << source << " => " << value.toString() << '\n';
}

} // namespace

int main() {
    novac::controllers::EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};

    atomics.integer("IntegerLiteral", {"int", "i32", "i64"});
    atomics.floating("FloatLiteral", {"float", "f32", "f64"});
    atomics.stringLiteral("StringLiteral", {"string", "str", "char"});
    atomics.boolean("BooleanLiteral", "yes", "no");

    atomics.add("plus");
    atomics.subtract("minus");
    atomics.multiply("mul");
    atomics.divide("div");
    atomics.modulo("mod");

    atomics.equal("is");
    atomics.lessEqual("at_most");
    atomics.greater("above");

    atomics.logicalAnd("and");
    atomics.logicalOr("or");
    atomics.logicalNot("not");
    atomics.negate("neg");

    std::cout << "== atomic expressions with custom tokens ==\n";

    printExpression(engine, "10_i32 plus 20_int mul 3_i64");
    printExpression(engine, "3.5_f32 plus 2.25_float");
    printExpression(engine, "neg 10_int plus 4_i32");
    printExpression(engine, "10_i64 above 3_int and yes");
    printExpression(engine, "not no or no");
    printExpression(engine, "10_int at_most 10_i32");
    printExpression(engine, "42_i64 is 42_int");
    printExpression(engine, "\"nova\"_str");

    return 0;
}