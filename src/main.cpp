#include <iostream>
#include <string>

#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/engine/EngineController.hpp"

int main() {
    novac::controllers::EngineController engine{};

    novac::assets::atomic::AtomicController atomics{engine};

    atomics.use(novac::assets::atomic::literals::standard());

    atomics.use(novac::assets::atomic::operations::numeric({
        .add = "+",
        .subtract = "-",
        .multiply = "*",
        .divide = "/",
        .modulo = "%",
        .negate = "-"
    }));

    atomics.use(novac::assets::atomic::operations::comparison({
        .equal = "==",
        .notEqual = "!=",
        .less = "<",
        .lessEqual = "<=",
        .greater = ">",
        .greaterEqual = ">="
    }));

    atomics.use(novac::assets::atomic::operations::logical({
        .andToken = "&&",
        .orToken = "||",
        .notToken = "!"
    }));

    novac::assets::essentials::EssentialsControllerOptions essentialsOptions{};

    essentialsOptions.core.programDomain = "program";
    essentialsOptions.core.statementDomain = "stmt";
    essentialsOptions.core.expressionDomain = "expr";

    essentialsOptions.core.programNodeKind = "Program";
    essentialsOptions.core.blockNodeKind = "BlockStmt";
    essentialsOptions.core.expressionStatementNodeKind = "ExpressionStatement";

    essentialsOptions.core.statementsField = "statements";
    essentialsOptions.core.expressionField = "expression";

    essentialsOptions.core.semicolonToken = ";";
    essentialsOptions.core.commaToken = ",";
    essentialsOptions.core.leftBraceToken = "{";
    essentialsOptions.core.rightBraceToken = "}";
    essentialsOptions.core.leftParenToken = "(";
    essentialsOptions.core.rightParenToken = ")";

    essentialsOptions.variables.declarationNodeKind = "VariableDeclaration";
    essentialsOptions.variables.expressionNodeKind = "VariableExpression";
    essentialsOptions.variables.assignmentNodeKind = "AssignmentStatement";
    essentialsOptions.variables.letKeyword = "let";
    essentialsOptions.variables.assignToken = "=";
    essentialsOptions.variables.nameField = "name";
    essentialsOptions.variables.valueField = "value";

    essentialsOptions.controlFlow.ifNodeKind = "IfStatement";
    essentialsOptions.controlFlow.whileNodeKind = "WhileStatement";
    essentialsOptions.controlFlow.forNodeKind = "ForStatement";
    essentialsOptions.controlFlow.ifKeyword = "if";
    essentialsOptions.controlFlow.elseKeyword = "else";
    essentialsOptions.controlFlow.whileKeyword = "while";
    essentialsOptions.controlFlow.forKeyword = "for";
    essentialsOptions.controlFlow.conditionField = "condition";
    essentialsOptions.controlFlow.thenField = "thenBranch";
    essentialsOptions.controlFlow.elseField = "elseBranch";
    essentialsOptions.controlFlow.bodyField = "body";
    essentialsOptions.controlFlow.initializerField = "initializer";
    essentialsOptions.controlFlow.stepField = "step";
    essentialsOptions.controlFlow.maxLoopIterations = 100000;

    essentialsOptions.functions.declarationNodeKind = "FunctionDeclaration";
    essentialsOptions.functions.callNodeKind = "FunctionCall";
    essentialsOptions.functions.parameterNodeKind = "FunctionParameter";
    essentialsOptions.functions.returnNodeKind = "ReturnStatement";
    essentialsOptions.functions.functionKeyword = "func";
    essentialsOptions.functions.returnKeyword = "return";
    essentialsOptions.functions.printFunctionName = "print";
    essentialsOptions.functions.mainFunctionName = "main";
    essentialsOptions.functions.nameField = "name";
    essentialsOptions.functions.bodyField = "body";
    essentialsOptions.functions.parametersField = "parameters";
    essentialsOptions.functions.argumentsField = "arguments";
    essentialsOptions.functions.valueField = "value";

    essentialsOptions.enforceChildTraits = false;

    novac::assets::essentials::EssentialsController essentials{
        engine,
        essentialsOptions
    };

    essentials.installStandardCore();

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