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

using novac::assets::essentials::EssentialInfo;
using novac::assets::essentials::scopes::ScopedBlocksFeature;
using novac::controllers::EngineController;

TEST(ScopedBlocksFeature, InfoReturnsCorrectMetadata) {
    ScopedBlocksFeature feature{};
    EssentialInfo info{feature.info()};

    CHECK(info.id == "essentials.scopes.blocks");
    CHECK(info.version == "0.1.0");
    CHECK(info.description == "Lexical scoped blocks");
    CHECK(info.nodeKinds.size() == 1);
    CHECK(info.nodeKinds[0] == "BlockStmt");
}

TEST(ScopedBlocksFeature, PackContainsFeature) {
    auto pack{novac::assets::essentials::scopes::scopedBlocks()};

    CHECK(pack.features.size() == 1);
    CHECK(pack.features[0] != nullptr);
    CHECK(pack.features[0]->info().id == "essentials.scopes.blocks");
}

TEST(ScopedBlocksFeature, StandardPackContainsFeature) {
    auto pack{novac::assets::essentials::scopes::standard()};

    CHECK(pack.features.size() == 1);
    CHECK(pack.features[0]->info().id == "essentials.scopes.blocks");
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

    CHECK(throwsRuntimeError([&]() {engine.eval(*program);}));
}

TEST(ScopedBlocksFeature, UnterminatedBlockThrows) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installProgram();
    essentials.installScopedBlocks();
    essentials.installVariables();

    CHECK(throwsRuntimeError([&]() {
        (void)engine.parse(R"(
{
    let x = 42;
)");
    }));
}

TEST(ScopedBlocksFeature, ReturnStopsRemainingStatements) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func main() {
    return 42;
    unknown;
}
)")};

    CHECK(engine.eval(*program).asInt() == 42);
}

TEST(ScopedBlocksFeature, CustomBlockSyntaxExecutes) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsControllerOptions options{};
    options.core.leftBraceToken = "[";
    options.core.rightBraceToken = "]";
    options.core.blockNodeKind = "Suite";
    options.core.statementsField = "items";
    novac::assets::essentials::EssentialsController essentials{engine, options};
    essentials.installProgram();
    essentials.installScopedBlocks();
    essentials.installVariables();

    const auto program{engine.parse(R"(
[
    let x = 42;
]
)")};

    engine.validate(*program);
    CHECK(engine.eval(*program).toString() == "void");
}

} // namespace
