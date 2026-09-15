#include "../../tester.hpp"

#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/assets/essentials/controlflow/ForLoops.hpp"
#include "novac/assets/essentials/functions/Functions.hpp"
#include "novac/assets/essentials/scopes/ScopedBlocks.hpp"
#include "novac/assets/essentials/variables/Variables.hpp"
#include "novac/engine/EngineController.hpp"

#include <functional>
#include <string>
#include <vector>

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

bool optionsThrow(const std::function<void(EssentialsControllerOptions &)> &mutator) {
    EngineController engine{};
    EssentialsControllerOptions options{};
    mutator(options);
    return throwsRuntimeError([&]() {EssentialsController controller{engine, options};});
}

TEST(EssentialsController, DefaultConstruction) {
    EngineController engine{};
    EssentialsController controller{engine};

    CHECK(&controller.engine() == &engine);
    CHECK(controller.core().programDomain == "program");
    CHECK(controller.core().statementDomain == "stmt");
    CHECK(controller.core().expressionDomain == "expr");
    CHECK(engine.startDomain() == "program");
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

TEST(EssentialsController, ConstAccessorsExposeConfiguredOptions) {
    EngineController engine{};
    EssentialsControllerOptions options{};
    options.enforceChildTraits = true;
    options.core.programNodeKind = "Root";
    options.variables.declarationNodeKind = "Binding";
    options.controlFlow.ifNodeKind = "Conditional";
    options.functions.declarationNodeKind = "Routine";

    EssentialsController controller{engine, options};
    const EssentialsController &view{controller};

    CHECK(&view.engine() == &engine);
    CHECK(view.options().enforceChildTraits);
    CHECK(view.core().programNodeKind == "Root");
    CHECK(view.variables().declarationNodeKind == "Binding");
    CHECK(view.controlFlow().ifNodeKind == "Conditional");
    CHECK(view.functions().declarationNodeKind == "Routine");
}

TEST(EssentialsController, EmptyCoreOptionsThrow) {
    const std::vector<std::function<void(EssentialsControllerOptions &)>> cases{
        [](auto &options) {options.core.programDomain.clear();},
        [](auto &options) {options.core.statementDomain.clear();},
        [](auto &options) {options.core.expressionDomain.clear();},
        [](auto &options) {options.core.programNodeKind.clear();},
        [](auto &options) {options.core.blockNodeKind.clear();},
        [](auto &options) {options.core.expressionStatementNodeKind.clear();},
        [](auto &options) {options.core.statementsField.clear();},
        [](auto &options) {options.core.expressionField.clear();},
        [](auto &options) {options.core.semicolonToken.clear();},
        [](auto &options) {options.core.commaToken.clear();},
        [](auto &options) {options.core.leftBraceToken.clear();},
        [](auto &options) {options.core.rightBraceToken.clear();},
        [](auto &options) {options.core.leftParenToken.clear();},
        [](auto &options) {options.core.rightParenToken.clear();}
    };

    for (const auto &mutator : cases)
        CHECK(optionsThrow(mutator));
}

TEST(EssentialsController, EmptyVariableOptionsThrow) {
    const std::vector<std::function<void(EssentialsControllerOptions &)>> cases{
        [](auto &options) {options.variables.declarationNodeKind.clear();},
        [](auto &options) {options.variables.expressionNodeKind.clear();},
        [](auto &options) {options.variables.assignmentNodeKind.clear();},
        [](auto &options) {options.variables.letKeyword.clear();},
        [](auto &options) {options.variables.assignToken.clear();},
        [](auto &options) {options.variables.nameField.clear();},
        [](auto &options) {options.variables.valueField.clear();}
    };

    for (const auto &mutator : cases)
        CHECK(optionsThrow(mutator));
}

TEST(EssentialsController, EmptyControlFlowOptionsThrow) {
    const std::vector<std::function<void(EssentialsControllerOptions &)>> cases{
        [](auto &options) {options.controlFlow.ifNodeKind.clear();},
        [](auto &options) {options.controlFlow.whileNodeKind.clear();},
        [](auto &options) {options.controlFlow.forNodeKind.clear();},
        [](auto &options) {options.controlFlow.ifKeyword.clear();},
        [](auto &options) {options.controlFlow.elseKeyword.clear();},
        [](auto &options) {options.controlFlow.whileKeyword.clear();},
        [](auto &options) {options.controlFlow.forKeyword.clear();},
        [](auto &options) {options.controlFlow.conditionField.clear();},
        [](auto &options) {options.controlFlow.thenField.clear();},
        [](auto &options) {options.controlFlow.elseField.clear();},
        [](auto &options) {options.controlFlow.bodyField.clear();},
        [](auto &options) {options.controlFlow.initializerField.clear();},
        [](auto &options) {options.controlFlow.stepField.clear();}
    };

    for (const auto &mutator : cases)
        CHECK(optionsThrow(mutator));
}

TEST(EssentialsController, EmptyFunctionOptionsThrow) {
    const std::vector<std::function<void(EssentialsControllerOptions &)>> cases{
        [](auto &options) {options.functions.declarationNodeKind.clear();},
        [](auto &options) {options.functions.callNodeKind.clear();},
        [](auto &options) {options.functions.parameterNodeKind.clear();},
        [](auto &options) {options.functions.returnNodeKind.clear();},
        [](auto &options) {options.functions.functionKeyword.clear();},
        [](auto &options) {options.functions.returnKeyword.clear();},
        [](auto &options) {options.functions.mainFunctionName.clear();},
        [](auto &options) {options.functions.nameField.clear();},
        [](auto &options) {options.functions.bodyField.clear();},
        [](auto &options) {options.functions.parametersField.clear();},
        [](auto &options) {options.functions.argumentsField.clear();},
        [](auto &options) {options.functions.valueField.clear();}
    };

    for (const auto &mutator : cases)
        CHECK(optionsThrow(mutator));
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

TEST(EssentialsController, InstallProgramRegistersFeature) {
    EngineController engine{};
    EssentialsController controller{engine};

    controller.installProgram();

    CHECK(controller.hasFeature("essentials.program"));
    CHECK(controller.features().size() == 1);
    CHECK(controller.features()[0].id == "essentials.program");
}

TEST(EssentialsController, InstallProgramDuplicateThrows) {
    EngineController engine{};
    EssentialsController controller{engine};

    controller.installProgram();

    CHECK(throwsRuntimeError([&]() {controller.installProgram();}));
}

TEST(EssentialsController, StandardPackContainsExpectedFeatures) {
    auto pack{novac::assets::essentials::essentials::standard()};

    CHECK(pack.features.size() == 9);
    CHECK(pack.features[0]->info().id == "essentials.scopes.blocks");
    CHECK(pack.features[1]->info().id == "essentials.variables");
    CHECK(pack.features[2]->info().id == "essentials.variables.expression-statements");
    CHECK(pack.features[3]->info().id == "essentials.controlflow.if");
    CHECK(pack.features[4]->info().id == "essentials.controlflow.while");
    CHECK(pack.features[5]->info().id == "essentials.controlflow.for");
    CHECK(pack.features[6]->info().id == "essentials.functions.return");
    CHECK(pack.features[7]->info().id == "essentials.functions");
    CHECK(pack.features[8]->info().id == "essentials.functions.main");
}

TEST(EssentialsController, InstallStandardCoreRegistersExpectedFeatures) {
    EngineController engine{};
    EssentialsController controller{engine};

    controller.installStandardCore();

    CHECK(controller.features().size() == 10);
    CHECK(controller.hasFeature("essentials.program"));
    CHECK(controller.hasFeature("essentials.scopes.blocks"));
    CHECK(controller.hasFeature("essentials.variables"));
    CHECK(controller.hasFeature("essentials.variables.expression-statements"));
    CHECK(controller.hasFeature("essentials.controlflow.if"));
    CHECK(controller.hasFeature("essentials.controlflow.while"));
    CHECK(controller.hasFeature("essentials.controlflow.for"));
    CHECK(controller.hasFeature("essentials.functions.return"));
    CHECK(controller.hasFeature("essentials.functions"));
    CHECK(controller.hasFeature("essentials.functions.main"));
}

TEST(EssentialsController, StandardSubpacksContainExpectedFeatures) {
    auto scopes{novac::assets::essentials::scopes::standard()};
    auto variables{novac::assets::essentials::variables::standard()};
    auto controlFlow{novac::assets::essentials::controlflow::standard()};
    auto functions{novac::assets::essentials::functions::standard()};

    CHECK(scopes.features.size() == 1);
    CHECK(variables.features.size() == 2);
    CHECK(controlFlow.features.size() == 3);
    CHECK(functions.features.size() == 3);
}

} // namespace
