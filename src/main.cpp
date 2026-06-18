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

    atomics.use(novac::assets::atomic::literals::integer(
            "IntegerLiteral",
            {"int", "i32", "i64"}
        )
    );

    atomics.use(novac::assets::atomic::literals::floating(
            "FloatLiteral",
            {"float", "f32", "f64"}
        )
    );

    atomics.use(novac::assets::atomic::literals::stringLiteral(
            "StringLiteral",
            {"string", "str", "char"}
        )
    );

    atomics.use(novac::assets::atomic::literals::boolean(
            "BooleanLiteral",
            "yes",
            "no"
        )
    );

    atomics.use(novac::assets::atomic::operations::numeric({
            .add = "plus",
            .subtract = "minus",
            .multiply = "mul",
            .divide = "div",
            .modulo = "mod",
            .negate = "neg"
        })
    );

    atomics.use(novac::assets::atomic::operations::comparison({
            .equal = "is",
            .lessEqual = "at_most",
            .greater = "above"
        })
    );

    atomics.use(novac::assets::atomic::operations::logical({
            .andToken = "and",
            .orToken = "or",
            .notToken = "not"
        })
    );

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