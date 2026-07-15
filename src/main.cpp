#include <iostream>
#include <string>

#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/engine/EngineController.hpp"


int main() {

    novac::controllers::EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};

    atomics.use(novac::assets::atomic::literals::standard());
    atomics.use(
        novac::assets::atomic::operations::numeric({
            .add = "+",
            .subtract = "-",
            .multiply = "*",
            .divide = "/",
            .modulo = "%",
            .negate = "-"
        })
    );

    atomics.use(
        novac::assets::atomic::operations::comparison({
            .equal = "==",
            .notEqual = "!=",
            .less = "<",
            .lessEqual = "<=",
            .greater = ">",
            .greaterEqual = ">="
        })
    );

    atomics.use(
        novac::assets::atomic::operations::logical({
            .andToken = "&&",
            .orToken = "||",
            .notToken = "!"
        })
    );

    novac::assets::essentials::EssentialsControllerOptions options{};

    options.core.programDomain = "program";
    options.core.statementDomain = "stmt";
    options.core.expressionDomain = "expr";

    options.core.programNodeKind = "Program";
    options.core.blockNodeKind = "BlockStmt";
    options.core.expressionStatementNodeKind = "ExpressionStatement";

    options.core.statementsField = "statements";
    options.core.expressionField = "expression";

    options.core.semicolonToken = ";";
    options.core.commaToken = ",";
    options.core.leftBraceToken = "{";
    options.core.rightBraceToken = "}";
    options.core.leftParenToken = "(";
    options.core.rightParenToken = ")";

    options.functions.functionKeyword = "func";
    options.functions.returnKeyword = "return";
    options.functions.mainFunctionName = "main";

    novac::assets::essentials::EssentialsController essentials{engine, options};

    essentials.installStandardCore();
    essentials.functionRegistry().native("print", [](const novac::ast::NodeList &arguments, novac::runtime::RuntimeContext &context) -> novac::runtime::Value {
            for(const auto &argument : arguments) {
                std::cout << context.eval(*argument).toString();
            }

            std::cout << '\n';
            return novac::runtime::Value::voidValue();
        }
    );

    essentials.functionRegistry().native("toto", [](const novac::ast::NodeList &, novac::runtime::RuntimeContext &) -> novac::runtime::Value {
            std::cout << "toto" << '\n';
            return novac::runtime::Value::voidValue();
        }
    );

    const std::string source{
R"(

func add(a, b) {
    return a + b;
}

func factorial(n) {

    result = 1;

    while n > 1 {

        result = result * n;
        n = n - 1;

    }

    return result;
}

func main() {

    x = 5;
    y = 6;

    z = add(x, y);

    print("x =");
    print(x);

    print("y =");
    print(y);

    print("z =");
    print(z);

    if z > 10 {

        print("greater than ten");

    } else {

        print("ten or below");

    }

    sum = 0;

    for(i = 0; i < 5; i = i + 1) {

        sum = sum + i;

    }

    print("sum =");
    print(sum);

    print("factorial =");
    print(factorial(5));

    toto();

    return z + sum;
}

)"

    };

    const novac::ast::NodePtr program{engine.parse(source)};

    engine.validate(*program);

    const novac::runtime::Value result{engine.eval(*program)};

    std::cout << "Program returned: " << result.toString() << '\n';

    return 0;
}