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

TEST(FunctionsFeature, NativeFunctionExecutes) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();

    essentials.functionRegistry().native(
        "answer",
        [](const novac::ast::NodeList &, novac::runtime::RuntimeContext &) {
            return novac::runtime::Value::integer(42);
        }
    );

    const auto program{engine.parse(R"(
func main() {
    return answer();
}
)")};

    engine.validate(*program);
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

    CHECK(throwsRuntimeError([&]() {
        engine.eval(*program);
    }));
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

    CHECK(throwsRuntimeError([&]() {
        engine.eval(*program);
    }));
}

} // namespace
