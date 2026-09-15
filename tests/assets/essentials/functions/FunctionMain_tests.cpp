#include "../../../tester.hpp"

#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/assets/essentials/functions/FunctionMain.hpp"
#include "novac/engine/EngineController.hpp"

namespace {

using novac::assets::essentials::EssentialInfo;
using novac::assets::essentials::functions::FunctionMainFeature;
using novac::controllers::EngineController;

TEST(FunctionMainFeature, InfoReturnsCorrectMetadata) {
    FunctionMainFeature feature{};
    EssentialInfo info{feature.info()};

    CHECK(info.id == "essentials.functions.main");
    CHECK(info.version == "0.1.0");
    CHECK(info.description == "Program entry point");
    CHECK(info.nodeKinds.empty());
    CHECK(info.traits.empty());
    CHECK(info.requirements.empty());
}

TEST(FunctionMainFeature, PackContainsFeature) {
    auto pack{novac::assets::essentials::functions::functionMain()};

    CHECK(pack.features.size() == 1);
    CHECK(pack.features[0]->info().id == "essentials.functions.main");
}

TEST(FunctionMainFeature, InstallsDefaultEntryPoint) {
    EngineController engine{};
    novac::assets::essentials::EssentialsController essentials{engine};

    essentials.use(novac::assets::essentials::functions::functionMain());

    CHECK(essentials.functionRegistry().entryPoint() == "main");
}

TEST(FunctionMainFeature, InstallsConfiguredEntryPoint) {
    EngineController engine{};
    novac::assets::essentials::EssentialsControllerOptions options{};
    options.functions.mainFunctionName = "start";
    novac::assets::essentials::EssentialsController essentials{engine, options};

    essentials.use(novac::assets::essentials::functions::functionMain());

    CHECK(essentials.functionRegistry().entryPoint() == "start");
}

TEST(FunctionMainFeature, ConfiguredEntryPointExecutes) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsControllerOptions options{};
    options.functions.mainFunctionName = "start";
    novac::assets::essentials::EssentialsController essentials{engine, options};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func start() {
    return 42;
}
)")};

    engine.validate(*program);
    CHECK(engine.eval(*program).asInt() == 42);
}

} // namespace
