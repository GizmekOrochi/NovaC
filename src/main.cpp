#include <iostream>
#include <string>

#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/engine/EngineController.hpp"

namespace {

void printExpression(
    const novac::controllers::EngineController &engine,
    const std::string &source
) {
    const novac::ast::NodePtr root{engine.parse(source)};

    engine.validate(*root);

    const novac::runtime::Value value{engine.eval(*root)};

    std::cout << source << " => " << value.toString() << '\n';
}

novac::ast::NodePtr printStmt(
    novac::controllers::EngineController &engine,
    const std::string &expressionSource
) {
    novac::ast::NodePtr statement{engine.makeNode("PrintStatement")};
    statement->set("expression", engine.parse(expressionSource));
    return statement;
}

novac::ast::NodePtr blockStmt(
    novac::controllers::EngineController &engine,
    novac::ast::NodeList statements
) {
    novac::ast::NodePtr block{engine.makeNode("BlockStmt")};
    block->set("statements", std::move(statements));
    return block;
}

void installPrintStatement(novac::controllers::EngineController &engine) {
    engine.node({
        "PrintStatement",
        {
            {"expression", novac::ast::FieldKind::Node, true, {}, {}}
        },
        {"Statement"},
        "Print expression statement."
    });

    engine.statement(
        "PrintStatement",
        [](const novac::ast::Node &node, novac::runtime::RuntimeContext &context) {
            const novac::runtime::Value value{
                context.eval(*node.child("expression"))
            };

            std::cout << "print => " << value.toString() << '\n';
        }
    );
}

void runNovaProgramDemo(novac::controllers::EngineController &engine) {
    std::cout << "\n== Nova program demo ==\n";

    /*
        Future Nova source equivalent:

        {
            print 10_i32 plus 20_int mul 3_i64;

            {
                print "Hello Nova"_str;
                print not no;
            }

            print 10_i64 above 3_int and yes;
        }
    */

    novac::ast::NodePtr root{
        blockStmt(
            engine,
            {
                printStmt(engine, "10_i32 plus 20_int mul 3_i64"),

                blockStmt(
                    engine,
                    {
                        printStmt(engine, "\"Hello Nova\"_str"),
                        printStmt(engine, "not no")
                    }
                ),

                printStmt(engine, "10_i64 above 3_int and yes")
            }
        )
    };

    engine.validate(*root);
    engine.exec(*root);
}

} // namespace

int main() {
    novac::controllers::EngineController engine{};

    novac::assets::atomic::AtomicController atomics{engine};
    novac::assets::essentials::EssentialsController essentials{engine};

    essentials.installStandardScopes();

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

    installPrintStatement(engine);

    std::cout << "== atomic expressions with custom tokens ==\n";

    printExpression(engine, "10_i32 plus 20_int mul 3_i64");
    printExpression(engine, "3.5_f32 plus 2.25_float");
    printExpression(engine, "neg 10_int plus 4_i32");
    printExpression(engine, "10_i64 above 3_int and yes");
    printExpression(engine, "not no or no");
    printExpression(engine, "10_int at_most 10_i32");
    printExpression(engine, "42_i64 is 42_int");
    printExpression(engine, "\"nova\"_str");

    runNovaProgramDemo(engine);

    return 0;
}