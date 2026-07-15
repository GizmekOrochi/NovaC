#include "../../../tester.hpp"

#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/engine/EngineController.hpp"

namespace {

using novac::controllers::EngineController;

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

} // namespace
