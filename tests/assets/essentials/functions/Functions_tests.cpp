#include "../../../tester.hpp"

#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/assets/essentials/functions/Functions.hpp"
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
using novac::assets::essentials::functions::FunctionsFeature;
using novac::controllers::EngineController;

TEST(FunctionsFeature, InfoReturnsCorrectMetadata) {
    FunctionsFeature feature{};
    EssentialInfo info{feature.info()};

    CHECK(info.id == "essentials.functions");
    CHECK(info.version == "0.1.0");
    CHECK(info.description == "Function declarations and calls");
    CHECK(info.nodeKinds.size() == 3);
    CHECK(info.nodeKinds[0] == "FunctionDeclaration");
    CHECK(info.nodeKinds[1] == "FunctionCall");
    CHECK(info.nodeKinds[2] == "FunctionParameter");
}

TEST(FunctionsFeature, PackContainsFeature) {
    auto pack{novac::assets::essentials::functions::functions()};

    CHECK(pack.features.size() == 1);
    CHECK(pack.features[0]->info().id == "essentials.functions");
}

TEST(FunctionsFeature, StandardPackContainsFunctionFeatures) {
    auto pack{novac::assets::essentials::functions::standard()};

    CHECK(pack.features.size() == 3);
    CHECK(pack.features[0]->info().id == "essentials.functions.return");
    CHECK(pack.features[1]->info().id == "essentials.functions");
    CHECK(pack.features[2]->info().id == "essentials.functions.main");
}

TEST(FunctionsFeature, UserFunctionExecutes) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();
    atomics.add();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installProgram();
    essentials.installScopedBlocks();
    essentials.installVariables();
    essentials.installExpressionStatements();
    essentials.installReturnStatements();
    essentials.installFunctions();

    const auto program{engine.parse(R"(
func add(a, b) {
    return a + b;
}

func main() {
    return add(20, 22);
}
)")};

    engine.validate(*program);
    CHECK(engine.eval(*program).asInt() == 42);
}

TEST(FunctionsFeature, ZeroArgumentFunctionExecutes) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func answer() {
    return 42;
}

func main() {
    return answer();
}
)")};

    CHECK(engine.eval(*program).asInt() == 42);
}

TEST(FunctionsFeature, FunctionWithoutReturnProducesVoid) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func noop() {
    let value = 42;
}

func main() {
    return noop();
}
)")};

    CHECK(engine.eval(*program).toString() == "void");
}

TEST(FunctionsFeature, RecursiveFunctionExecutes) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.standardCore();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func factorial(n) {
    if n <= 1 {
        return 1;
    }

    return n * factorial(n - 1);
}

func main() {
    return factorial(5);
}
)")};

    engine.validate(*program);
    CHECK(engine.eval(*program).asInt() == 120);
}

TEST(FunctionsFeature, ArgumentsAreEvaluatedInCallerContext) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func select(first, second) {
    return second;
}

func main() {
    let first = 42;
    return select(1, first);
}
)")};

    CHECK(engine.eval(*program).asInt() == 42);
}

TEST(FunctionsFeature, ParameterScopeDoesNotLeak) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func identity(value) {
    return value;
}

func main() {
    let value = 10;
    identity(42);
    return value;
}
)")};

    CHECK(engine.eval(*program).asInt() == 10);
}

TEST(FunctionsFeature, NativeFunctionExecutes) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();
    essentials.functionRegistry().native("answer", [](const novac::ast::NodeList &, novac::runtime::RuntimeContext &) {return novac::runtime::Value::integer(42);});

    const auto program{engine.parse(R"(
func main() {
    return answer();
}
)")};

    engine.validate(*program);
    CHECK(engine.eval(*program).asInt() == 42);
}

TEST(FunctionsFeature, NativeFunctionReceivesArguments) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();
    essentials.functionRegistry().native("first", [](const novac::ast::NodeList &arguments, novac::runtime::RuntimeContext &context) {
        if (arguments.empty())
            return novac::runtime::Value::voidValue();
        return context.eval(*arguments[0]);
    });

    const auto program{engine.parse(R"(
func main() {
    return first(42);
}
)")};

    CHECK(engine.eval(*program).asInt() == 42);
}

TEST(FunctionsFeature, UnknownFunctionThrows) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func main() {
    missing();
}
)")};

    CHECK(throwsRuntimeError([&]() {engine.eval(*program);}));
}

TEST(FunctionsFeature, WrongArgumentCountThrows) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func identity(value) {
    return value;
}

func main() {
    return identity();
}
)")};

    CHECK(throwsRuntimeError([&]() {engine.eval(*program);}));
}

TEST(FunctionsFeature, CustomFunctionKeywordExecutes) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsControllerOptions options{};
    options.functions.functionKeyword = "fn";
    novac::assets::essentials::EssentialsController essentials{engine, options};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
fn main() {
    return 42;
}
)")};

    CHECK(engine.eval(*program).asInt() == 42);
}

TEST(FunctionsFeature, CustomFunctionSchemaExecutes) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsControllerOptions options{};
    options.functions.declarationNodeKind = "RoutineDeclaration";
    options.functions.callNodeKind = "RoutineCall";
    options.functions.parameterNodeKind = "RoutineParameter";
    options.functions.nameField = "identifier";
    options.functions.bodyField = "routineBody";
    options.functions.parametersField = "params";
    options.functions.argumentsField = "args";

    novac::assets::essentials::EssentialsController essentials{engine, options};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func identity(value) {
    return value;
}

func main() {
    return identity(42);
}
)")};

    engine.validate(*program);
    CHECK(engine.eval(*program).asInt() == 42);
}

} // namespace
