#include "../../tester.hpp"

#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/engine/EngineController.hpp"
#include "novac/engine/syntax/Node.hpp"

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

using novac::controllers::EngineController;

TEST(EssentialsProgram, ExecutesTopLevelStatementsWithoutMain) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installProgram();
    essentials.installVariables();

    const auto program{engine.parse("let answer = 42;")};

    engine.validate(*program);
    CHECK(engine.eval(*program).toString() == "void");
}

TEST(EssentialsProgram, TopLevelReturnStopsExecution) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installProgram();
    essentials.installReturnStatements();
    essentials.installVariables();

    const auto program{engine.parse(R"(
return 42;
let unreachable = 1;
)")};

    CHECK(engine.eval(*program).asInt() == 42);
}

TEST(EssentialsProgram, NullTopLevelNodeThrows) {
    EngineController engine{};
    novac::assets::essentials::EssentialsController essentials{engine};
    essentials.installProgram();

    auto program{novac::ast::Node::make("Program")};
    program->set("statements", novac::ast::NodeList{nullptr});

    CHECK(throwsRuntimeError([&]() {engine.eval(*program);}));
}

TEST(EssentialsProgram, CustomProgramSchemaExecutes) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsControllerOptions options{};
    options.core.programNodeKind = "SourceFile";
    options.core.statementsField = "items";
    novac::assets::essentials::EssentialsController essentials{engine, options};
    essentials.installProgram();
    essentials.installVariables();

    const auto program{engine.parse("let answer = 42;")};

    CHECK(program->kind() == "SourceFile");
    CHECK(program->list("items").size() == 1);
    engine.validate(*program);
    CHECK(engine.eval(*program).toString() == "void");
}

TEST(EssentialsProgram, CustomMainFunctionNameExecutes) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    atomics.integer();

    novac::assets::essentials::EssentialsControllerOptions options{};
    options.functions.mainFunctionName = "start";
    novac::assets::essentials::EssentialsController essentials{engine, options};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func start() {
    return 42;
}
)")};

    engine.validate(*program);
    CHECK(engine.eval(*program).asInt() == 42);
}

} // namespace
