#include "../../../tester.hpp"

#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/assets/essentials/controlflow/ForLoops.hpp"
#include "novac/assets/essentials/controlflow/IfStatements.hpp"
#include "novac/assets/essentials/controlflow/WhileLoops.hpp"
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
using novac::assets::essentials::controlflow::ForLoopsFeature;
using novac::assets::essentials::controlflow::IfStatementsFeature;
using novac::assets::essentials::controlflow::WhileLoopsFeature;
using novac::controllers::EngineController;

TEST(IfStatementsFeature, InfoReturnsCorrectMetadata) {
    IfStatementsFeature feature{};
    EssentialInfo info{feature.info()};

    CHECK(info.id == "essentials.controlflow.if");
    CHECK(info.version == "0.1.0");
    CHECK(info.description == "If and else statements");
    CHECK(info.nodeKinds.size() == 1);
    CHECK(info.nodeKinds[0] == "IfStatement");
}

TEST(WhileLoopsFeature, InfoReturnsCorrectMetadata) {
    WhileLoopsFeature feature{};
    EssentialInfo info{feature.info()};

    CHECK(info.id == "essentials.controlflow.while");
    CHECK(info.version == "0.1.0");
    CHECK(info.description == "While loops");
    CHECK(info.nodeKinds.size() == 1);
    CHECK(info.nodeKinds[0] == "WhileStatement");
}

TEST(ForLoopsFeature, InfoReturnsCorrectMetadata) {
    ForLoopsFeature feature{};
    EssentialInfo info{feature.info()};

    CHECK(info.id == "essentials.controlflow.for");
    CHECK(info.version == "0.1.0");
    CHECK(info.description == "For loops");
    CHECK(info.nodeKinds.size() == 1);
    CHECK(info.nodeKinds[0] == "ForStatement");
}

TEST(ControlFlow, IndividualPacksContainExpectedFeatures) {
    auto ifPack{novac::assets::essentials::controlflow::ifStatements()};
    auto whilePack{novac::assets::essentials::controlflow::whileLoops()};
    auto forPack{novac::assets::essentials::controlflow::forLoops()};

    CHECK(ifPack.features.size() == 1);
    CHECK(whilePack.features.size() == 1);
    CHECK(forPack.features.size() == 1);
    CHECK(ifPack.features[0]->info().id == "essentials.controlflow.if");
    CHECK(whilePack.features[0]->info().id == "essentials.controlflow.while");
    CHECK(forPack.features[0]->info().id == "essentials.controlflow.for");
}

TEST(ControlFlow, StandardPackContainsAllFeatures) {
    auto pack{novac::assets::essentials::controlflow::standard()};

    CHECK(pack.features.size() == 3);
    CHECK(pack.features[0]->info().id == "essentials.controlflow.if");
    CHECK(pack.features[1]->info().id == "essentials.controlflow.while");
    CHECK(pack.features[2]->info().id == "essentials.controlflow.for");
}

TEST(ControlFlow, IfTrueBranchExecutes) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.standardCore();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func main() {
    let result = 0;

    if true {
        result = 42;
    } else {
        result = 1;
    }

    return result;
}
)")};

    engine.validate(*program);
    CHECK(engine.eval(*program).asInt() == 42);
}

TEST(ControlFlow, IfFalseBranchExecutes) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.standardCore();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func main() {
    let result = 0;

    if false {
        result = 1;
    } else {
        result = 42;
    }

    return result;
}
)")};

    engine.validate(*program);
    CHECK(engine.eval(*program).asInt() == 42);
}

TEST(ControlFlow, IfFalseWithoutElseDoesNothing) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.standardCore();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func main() {
    let result = 42;

    if false {
        result = 1;
    }

    return result;
}
)")};

    engine.validate(*program);
    CHECK(engine.eval(*program).asInt() == 42);
}

TEST(ControlFlow, WhileLoopExecutes) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.standardCore();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func main() {
    let value = 0;

    while value < 5 {
        value = value + 1;
    }

    return value;
}
)")};

    engine.validate(*program);
    CHECK(engine.eval(*program).asInt() == 5);
}

TEST(ControlFlow, WhileReturnStopsLoop) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.standardCore();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func main() {
    while true {
        return 42;
    }

    return 0;
}
)")};

    engine.validate(*program);
    CHECK(engine.eval(*program).asInt() == 42);
}

TEST(ControlFlow, WhileMaximumIterationCountThrows) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.boolean();

    novac::assets::essentials::EssentialsControllerOptions options{};
    options.controlFlow.maxLoopIterations = 3;
    novac::assets::essentials::EssentialsController essentials{engine, options};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func main() {
    while true {
    }
}
)")};

    CHECK(throwsRuntimeError([&]() {engine.eval(*program);}));
}

TEST(ControlFlow, ZeroLoopLimitDisablesWhileGuard) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.standardCore();

    novac::assets::essentials::EssentialsControllerOptions options{};
    options.controlFlow.maxLoopIterations = 0;
    novac::assets::essentials::EssentialsController essentials{engine, options};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func main() {
    let value = 0;

    while value < 5 {
        value = value + 1;
    }

    return value;
}
)")};

    CHECK(engine.eval(*program).asInt() == 5);
}

TEST(ControlFlow, ForLoopExecutes) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.standardCore();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func main() {
    let sum = 0;

    for(i = 0; i < 5; i = i + 1) {
        sum = sum + i;
    }

    return sum;
}
)")};

    engine.validate(*program);
    CHECK(engine.eval(*program).asInt() == 10);
}

TEST(ControlFlow, ForWithoutInitializerExecutes) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.standardCore();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func main() {
    let i = 0;

    for(; i < 5; i = i + 1) {
    }

    return i;
}
)")};

    CHECK(engine.eval(*program).asInt() == 5);
}

TEST(ControlFlow, ForWithoutConditionExecutesUntilReturn) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.standardCore();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func main() {
    for(i = 0; ; i = i + 1) {
        if i == 5 {
            return i;
        }
    }
}
)")};

    CHECK(engine.eval(*program).asInt() == 5);
}

TEST(ControlFlow, ForWithoutStepExecutes) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.standardCore();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func main() {
    let i = 0;

    for(i = 0; i < 5; ) {
        i = i + 1;
    }

    return i;
}
)")};

    CHECK(engine.eval(*program).asInt() == 5);
}

TEST(ControlFlow, ForMaximumIterationCountThrows) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsControllerOptions options{};
    options.controlFlow.maxLoopIterations = 3;
    novac::assets::essentials::EssentialsController essentials{engine, options};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func main() {
    for(i = 0; ; i = i) {
    }
}
)")};

    CHECK(throwsRuntimeError([&]() {engine.eval(*program);}));
}

TEST(ControlFlow, CustomKeywordsExecute) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.standardCore();

    novac::assets::essentials::EssentialsControllerOptions options{};
    options.controlFlow.ifKeyword = "when";
    options.controlFlow.elseKeyword = "otherwise";
    options.controlFlow.whileKeyword = "repeatwhile";
    options.controlFlow.forKeyword = "repeatfor";
    novac::assets::essentials::EssentialsController essentials{engine, options};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func main() {
    let result = 0;

    when true {
        result = 40;
    } otherwise {
        result = 1;
    }

    repeatfor(i = 0; i < 2; i = i + 1) {
        result = result + 1;
    }

    return result;
}
)")};

    CHECK(engine.eval(*program).asInt() == 42);
}

} // namespace
