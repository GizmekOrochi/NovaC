#include <iostream>
#include <string>

#include "novac/engine/EngineController.hpp"

#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/assets/atomic/AtomicPattern.hpp"

#include "novac/assets/atomic/literals/BooleanLiteralAtomic.hpp"
#include "novac/assets/atomic/literals/FloatLiteralAtomic.hpp"
#include "novac/assets/atomic/literals/IntegerLiteralAtomic.hpp"
#include "novac/assets/atomic/literals/StringLiteralAtomic.hpp"

#include "novac/assets/atomic/operations/ComparisonOperations.hpp"
#include "novac/assets/atomic/operations/LogicalOperations.hpp"
#include "novac/assets/atomic/operations/NumericOperations.hpp"

namespace {

void printExpression(
    const novac::controllers::EngineController& engine,
    const std::string& source)
{
    const novac::ast::NodePtr root{engine.parse(source)};
    engine.validate(*root);

    const novac::runtime::Value value{engine.eval(*root)};

    std::cout << source << " => " << value.toString() << '\n';
}

} // namespace

int main()
{
    using novac::assets::atomic::TokenPattern;

    novac::controllers::EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};

    auto integerLiteral{
        novac::assets::atomic::literals::IntegerLiteralAtomic{
            "IntegerLiteral",
            TokenPattern::suffixRegex(
                "$int",
                R"(_(int|i32|i64))")
        }
    };

    auto floatLiteral{
        novac::assets::atomic::literals::FloatLiteralAtomic{
            "FloatLiteral",
            TokenPattern::suffixRegex(
                "$float",
                R"(_(float|f32|f64))")
        }
    };

    auto stringLiteral{
        novac::assets::atomic::literals::StringLiteralAtomic{
            "StringLiteral",
            TokenPattern::suffixRegex(
                "$string",
                R"(_(string|str|char))")
        }
    };

    auto booleanLiteral{novac::assets::atomic::literals::BooleanLiteralAtomic{"BooleanLiteral", {.trueToken = "yes", .falseToken = "no"}}};

    auto add{novac::assets::atomic::operations::AddOperationAtomic{TokenPattern::text("plus")}};
    auto subtract{novac::assets::atomic::operations::SubtractOperationAtomic{TokenPattern::text("minus")}};
    auto multiply{novac::assets::atomic::operations::MultiplyOperationAtomic{TokenPattern::text("mul")}};
    auto divide{novac::assets::atomic::operations::DivideOperationAtomic{TokenPattern::text("div")}};
    auto modulo{novac::assets::atomic::operations::ModuloOperationAtomic{TokenPattern::text("mod")}};
    auto equal{novac::assets::atomic::operations::EqualOperationAtomic{TokenPattern::text("is")}};
    auto lessEqual{novac::assets::atomic::operations::LessEqualOperationAtomic{TokenPattern::text("at_most")}};
    auto greater{novac::assets::atomic::operations::GreaterOperationAtomic{TokenPattern::text("above")}};
    auto logicalAnd{novac::assets::atomic::operations::LogicalAndOperationAtomic{TokenPattern::text("and")}};
    auto logicalOr{novac::assets::atomic::operations::LogicalOrOperationAtomic{TokenPattern::text("or")}};
    auto logicalNot{novac::assets::atomic::operations::LogicalNotOperationAtomic{TokenPattern::text("not")}};
    auto negate{novac::assets::atomic::operations::NumericNegateOperationAtomic{TokenPattern::text("neg")}};

    atomics.use(integerLiteral);
    atomics.use(floatLiteral);
    atomics.use(stringLiteral);
    atomics.use(booleanLiteral);

    atomics.use(add);
    atomics.use(subtract);
    atomics.use(multiply);
    atomics.use(divide);
    atomics.use(modulo);

    atomics.use(equal);
    atomics.use(lessEqual);
    atomics.use(greater);

    atomics.use(logicalAnd);
    atomics.use(logicalOr);
    atomics.use(logicalNot);
    atomics.use(negate);

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