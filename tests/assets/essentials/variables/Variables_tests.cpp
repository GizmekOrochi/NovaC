#include "../../../tester.hpp"

#include "novac/assets/atomic/AtomicController.hpp"
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

using novac::assets::essentials::EssentialInfo;
using novac::assets::essentials::variables::VariablesFeature;
using novac::controllers::EngineController;

TEST(VariablesFeature, InfoReturnsCorrectMetadata) {
    VariablesFeature feature{};
    EssentialInfo info{feature.info()};

    CHECK(info.id == "essentials.variables");
    CHECK(info.version == "0.1.0");
    CHECK(info.description == "Variable declarations, assignments, and lookups");
    CHECK(info.nodeKinds.size() == 3);
    CHECK(info.nodeKinds[0] == "VariableDeclaration");
    CHECK(info.nodeKinds[1] == "VariableExpression");
    CHECK(info.nodeKinds[2] == "AssignmentStatement");
}

TEST(VariablesFeature, PackContainsFeature) {
    auto pack{novac::assets::essentials::variables::variables()};

    CHECK(pack.features.size() == 1);
    CHECK(pack.features[0]->info().id == "essentials.variables");
}

TEST(VariablesFeature, StandardPackContainsVariableFeatures) {
    auto pack{novac::assets::essentials::variables::standard()};

    CHECK(pack.features.size() == 2);
    CHECK(pack.features[0]->info().id == "essentials.variables");
    CHECK(pack.features[1]->info().id == "essentials.variables.expression-statements");
}

TEST(VariablesFeature, DeclarationExecutes) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installProgram();
    essentials.installVariables();

    const auto program{engine.parse("let x = 42;")};
    engine.validate(*program);

    CHECK(engine.eval(*program).toString() == "void");
}

TEST(VariablesFeature, DeclarationAndLookupExecute) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func main() {
    let x = 42;
    return x;
}
)")};

    engine.validate(*program);
    CHECK(engine.eval(*program).asInt() == 42);
}

TEST(VariablesFeature, AssignmentUpdatesExistingVariable) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func main() {
    let x = 1;
    x = 42;
    return x;
}
)")};

    CHECK(engine.eval(*program).asInt() == 42);
}

TEST(VariablesFeature, AssignmentCreatesMissingVariable) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func main() {
    x = 42;
    return x;
}
)")};

    CHECK(engine.eval(*program).asInt() == 42);
}

TEST(VariablesFeature, DuplicateDeclarationThrows) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func main() {
    let x = 1;
    let x = 2;
    return x;
}
)")};

    CHECK(throwsRuntimeError([&]() {engine.eval(*program);}));
}

TEST(VariablesFeature, UnknownVariableThrows) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installProgram();
    essentials.installVariables();
    essentials.installExpressionStatements();

    const auto program{engine.parse("unknown;")};

    CHECK(throwsRuntimeError([&]() {engine.eval(*program);}));
}

TEST(VariablesFeature, CustomSyntax) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsControllerOptions options{};
    options.variables.letKeyword = "var";
    options.variables.assignToken = ":=";

    novac::assets::essentials::EssentialsController essentials{engine, options};
    essentials.installProgram();
    essentials.installVariables();

    const auto program{engine.parse("var answer := 42;")};
    engine.validate(*program);
    CHECK(engine.eval(*program).toString() == "void");
}

TEST(VariablesFeature, CustomSchemaExecutes) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsControllerOptions options{};
    options.variables.declarationNodeKind = "BindingDeclaration";
    options.variables.expressionNodeKind = "BindingExpression";
    options.variables.assignmentNodeKind = "BindingAssignment";
    options.variables.nameField = "identifier";
    options.variables.valueField = "initializer";

    novac::assets::essentials::EssentialsController essentials{engine, options};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func main() {
    let answer = 1;
    answer = 42;
    return answer;
}
)")};

    engine.validate(*program);
    CHECK(engine.eval(*program).asInt() == 42);
}

} // namespace
