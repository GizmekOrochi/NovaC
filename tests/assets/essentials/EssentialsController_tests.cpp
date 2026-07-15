#include "../../tester.hpp"

#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/assets/essentials/variables/Variables.hpp"
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

using novac::assets::essentials::EssentialsController;
using novac::assets::essentials::EssentialsControllerOptions;
using novac::controllers::EngineController;

TEST(EssentialsController, DefaultConstruction) {
    EngineController engine{};
    EssentialsController controller{engine};

    CHECK(&controller.engine() == &engine);
    CHECK(controller.core().programDomain == "program");
    CHECK(controller.core().statementDomain == "stmt");
    CHECK(controller.core().expressionDomain == "expr");
}

TEST(EssentialsController, CustomOptionsConstruction) {
    EngineController engine{};
    EssentialsControllerOptions options{};

    options.core.programDomain = "source";
    options.core.statementDomain = "statement";
    options.core.expressionDomain = "expression";
    options.variables.letKeyword = "var";
    options.controlFlow.ifKeyword = "when";
    options.functions.functionKeyword = "fn";

    EssentialsController controller{engine, options};

    CHECK(controller.core().programDomain == "source");
    CHECK(controller.core().statementDomain == "statement");
    CHECK(controller.core().expressionDomain == "expression");
    CHECK(controller.variables().letKeyword == "var");
    CHECK(controller.controlFlow().ifKeyword == "when");
    CHECK(controller.functions().functionKeyword == "fn");
    CHECK(engine.startDomain() == "source");
}

TEST(EssentialsController, EmptyProgramDomainThrows) {
    EngineController engine{};
    EssentialsControllerOptions options{};
    options.core.programDomain.clear();

    CHECK(throwsRuntimeError([&]() {EssentialsController controller{engine, options};}));
}

TEST(EssentialsController, EmptyStatementDomainThrows) {
    EngineController engine{};
    EssentialsControllerOptions options{};
    options.core.statementDomain.clear();

    CHECK(throwsRuntimeError([&]() {EssentialsController controller{engine, options};}));
}

TEST(EssentialsController, EmptyExpressionDomainThrows) {
    EngineController engine{};
    EssentialsControllerOptions options{};
    options.core.expressionDomain.clear();

    CHECK(throwsRuntimeError([&]() {EssentialsController controller{engine, options};}));
}

TEST(EssentialsController, UseFeatureRemembersMetadata) {
    EngineController engine{};
    EssentialsController controller{engine};

    controller.use(novac::assets::essentials::variables::variables());

    CHECK(controller.hasFeature("essentials.variables"));
    CHECK(controller.features().size() == 1);
    CHECK(controller.features()[0].id == "essentials.variables");
}

TEST(EssentialsController, DuplicatePackFeatureThrows) {
    EngineController engine{};
    EssentialsController controller{engine};

    controller.use(novac::assets::essentials::variables::variables());

    CHECK(throwsRuntimeError([&]() {controller.use(novac::assets::essentials::variables::variables());}));
}

TEST(EssentialsController, OwnNullFeatureThrows) {
    EngineController engine{};
    EssentialsController controller{engine};

    CHECK(throwsRuntimeError([&]() {controller.own(nullptr);}));
}

TEST(EssentialsController, FunctionRegistryIsAccessible) {
    EngineController engine{};
    EssentialsController controller{engine};

    CHECK(&controller.functionRegistry() == &static_cast<const EssentialsController &>(controller).functionRegistry());
}

} // namespace
