#include "../../../tester.hpp"

#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/assets/essentials/variables/ExpressionStatements.hpp"
#include "novac/engine/EngineController.hpp"

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

using novac::assets::essentials::EssentialInfo;
using novac::assets::essentials::variables::ExpressionStatementsFeature;
using novac::controllers::EngineController;

TEST(ExpressionStatementsFeature, InfoReturnsCorrectMetadata) {
    ExpressionStatementsFeature feature{};
    EssentialInfo info{feature.info()};

    CHECK(info.id == "essentials.variables.expression-statements");
    CHECK(info.version == "0.1.0");
    CHECK(info.description == "Expression statements");
    CHECK(info.nodeKinds.size() == 1);
    CHECK(info.nodeKinds[0] == "ExpressionStatement");
}

TEST(ExpressionStatementsFeature, PackContainsFeature) {
    auto pack{novac::assets::essentials::variables::expressionStatements()};

    CHECK(pack.features.size() == 1);
    CHECK(pack.features[0]->info().id == "essentials.variables.expression-statements");
}

TEST(ExpressionStatementsFeature, IntegerExpressionExecutes) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installProgram();
    essentials.installExpressionStatements();

    const auto program{engine.parse("42;")};
    engine.validate(*program);

    CHECK(engine.eval(*program).toString() == "void");
}

TEST(ExpressionStatementsFeature, MissingSemicolonIsRejected) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installProgram();
    essentials.installExpressionStatements();

    CHECK(throwsRuntimeError([&]() {(void)engine.parse("42");}));
}

TEST(ExpressionStatementsFeature, CustomSyntaxExecutes) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsControllerOptions options{};
    options.core.expressionStatementNodeKind = "EvalStatement";
    options.core.expressionField = "payload";
    options.core.semicolonToken = "!";

    novac::assets::essentials::EssentialsController essentials{engine, options};
    essentials.installProgram();
    essentials.installExpressionStatements();

    const auto program{engine.parse("42!")};
    engine.validate(*program);
    CHECK(engine.eval(*program).toString() == "void");
}

} // namespace
