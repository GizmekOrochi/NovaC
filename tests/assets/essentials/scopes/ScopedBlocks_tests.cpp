#include "../../../tester.hpp"

#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/assets/essentials/scopes/ScopedBlocks.hpp"
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

using novac::assets::essentials::scopes::ScopedBlocksFeature;
using novac::controllers::EngineController;

TEST(ScopedBlocksFeature, PackContainsFeature) {
    auto pack{novac::assets::essentials::scopes::scopedBlocks()};

    CHECK(pack.features.size() == 1);
    CHECK(pack.features[0] != nullptr);
}

TEST(ScopedBlocksFeature, BlockExecutes) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installProgram();
    essentials.installScopedBlocks();
    essentials.installVariables();

    const auto program{engine.parse(R"(
{
    let x = 42;
}
)")};

    engine.validate(*program);
    CHECK(engine.eval(*program).toString() == "void");
}

TEST(ScopedBlocksFeature, LocalVariableDoesNotEscape) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installProgram();
    essentials.installScopedBlocks();
    essentials.installVariables();
    essentials.installExpressionStatements();

    const auto program{engine.parse(R"(
{
    let x = 42;
}

x;
)")};

    CHECK(throwsRuntimeError([&]() {
        engine.eval(*program);
    }));
}

} // namespace
