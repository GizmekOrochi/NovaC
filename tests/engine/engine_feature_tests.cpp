#include "../tester.hpp"
#include "novac/engine/EngineController.hpp"

#include <stdexcept>
#include <string>

namespace {

using novac::controllers::EngineController;
using novac::controllers::EngineFeature;

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

} // namespace

TEST(EngineFeature, ConstructsWithName) {
    EngineFeature feature{"core"};

    CHECK(feature.name() == "core");
    CHECK(feature.version() == "0.1.0");
    CHECK(feature.description().empty());
    CHECK(feature.capabilities().empty());
    CHECK(feature.requiredCapabilities().empty());
    CHECK(feature.conflicts().empty());
}

TEST(EngineFeature, RejectsEmptyName) {
    CHECK(throwsRuntimeError([]() { EngineFeature feature{""}; }));
}

TEST(EngineFeature, SetsVersion) {
    EngineFeature feature{"core"};

    EngineFeature &returned{feature.version("1.2.3")};

    CHECK(&returned == &feature);
    CHECK(feature.version() == "1.2.3");
}

TEST(EngineFeature, RejectsEmptyVersion) {
    EngineFeature feature{"core"};

    CHECK(throwsRuntimeError([&]() { feature.version(""); }));
}

TEST(EngineFeature, SetsDescription) {
    EngineFeature feature{"core"};

    EngineFeature &returned{feature.description("Core language features.")};

    CHECK(&returned == &feature);
    CHECK(feature.description() == "Core language features.");
}

TEST(EngineFeature, AddsProvidedCapabilities) {
    EngineFeature feature{"core"};

    EngineFeature &returned{feature.provides("lexer").provides("parser")};

    CHECK(&returned == &feature);
    CHECK(feature.capabilities().size() == 2);
    CHECK(feature.capabilities()[0] == "lexer");
    CHECK(feature.capabilities()[1] == "parser");
}

TEST(EngineFeature, RejectsEmptyProvidedCapability) {
    EngineFeature feature{"core"};

    CHECK(throwsRuntimeError([&]() { feature.provides(""); }));
}

TEST(EngineFeature, AddsRequiredCapabilities) {
    EngineFeature feature{"parser"};

    EngineFeature &returned{feature.requiresCapability("lexer").dependsOn("tokens")};

    CHECK(&returned == &feature);
    CHECK(feature.requiredCapabilities().size() == 2);
    CHECK(feature.requiredCapabilities()[0] == "lexer");
    CHECK(feature.requiredCapabilities()[1] == "tokens");
}

TEST(EngineFeature, RejectsEmptyRequiredCapability) {
    EngineFeature feature{"parser"};

    CHECK(throwsRuntimeError([&]() { feature.requiresCapability(""); }));
    CHECK(throwsRuntimeError([&]() { feature.dependsOn(""); }));
}

TEST(EngineFeature, AddsConflicts) {
    EngineFeature feature{"core"};

    EngineFeature &returned{feature.conflictsWith("legacy")};

    CHECK(&returned == &feature);
    CHECK(feature.conflicts().size() == 1);
    CHECK(feature.conflicts()[0] == "legacy");
}

TEST(EngineFeature, RejectsEmptyConflict) {
    EngineFeature feature{"core"};

    CHECK(throwsRuntimeError([&]() { feature.conflictsWith(""); }));
}

TEST(EngineFeature, InstallsCallbacksInOrder) {
    EngineFeature feature{"core"};
    std::string trace{};

    feature
        .onInstall([&](EngineController &) { trace += "A"; })
        .onInstall([&](EngineController &) { trace += "B";});

    EngineController engine{};
    feature.install(engine);

    CHECK(trace == "AB");
}

TEST(EngineFeature, RejectsEmptyInstaller) {
    EngineFeature feature{"core"};

    CHECK(throwsRuntimeError([&]() { feature.onInstall({}); }));
}
