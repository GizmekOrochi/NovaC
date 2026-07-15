#include "../../../tester.hpp"

#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/assets/essentials/functions/ReturnStatements.hpp"
#include "novac/engine/EngineController.hpp"

namespace {

using novac::assets::essentials::EssentialInfo;
using novac::assets::essentials::functions::ReturnStatementsFeature;
using novac::controllers::EngineController;

TEST(ReturnStatementsFeature, InfoReturnsCorrectMetadata) {
    ReturnStatementsFeature feature{};
    EssentialInfo info{feature.info()};

    CHECK(info.id == "essentials.functions.return");
    CHECK(info.version == "0.1.0");
    CHECK(info.description == "Return statements");
    CHECK(info.nodeKinds.size() == 1);
    CHECK(info.nodeKinds[0] == "ReturnStatement");
}

TEST(ReturnStatementsFeature, ReturnValueExecutes) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func main() {
    return 42;
}
)")};

    engine.validate(*program);
    CHECK(engine.eval(*program).asInt() == 42);
}

TEST(ReturnStatementsFeature, EmptyReturnProducesVoid) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func main() {
    return;
}
)")};

    engine.validate(*program);
    CHECK(engine.eval(*program).toString() == "void");
}

} // namespace
