#include "../../../tester.hpp"

#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/assets/essentials/variables/ExpressionStatements.hpp"
#include "novac/engine/EngineController.hpp"

namespace {

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

} // namespace
