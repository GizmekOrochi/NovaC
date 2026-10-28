// PARKED TESTS: not part of the default NovaC test target while the
// compilation asset is isolated for redesign. Do not automatically link them.
#include "../../tester.hpp"
#include "NovaC.hpp"
#include "novac/assets/compilation/Compilation.hpp"
#include "novac/assets/compilation/CompilationController.hpp"
#include "novac/assets/compilation/backend/Backend.hpp"

#include <memory>
#include <string>
#include <vector>

namespace {
using namespace novac;

class CountingBackend final : public backend::Backend {
public:
    void run(compilation::CompilationSession &session) const override {
        const int input{session.artifacts().require<int>("input")};
        session.artifacts().set<int>("output", input + 1);
    }
};

class ClassicPipelineBackend final : public backend::Backend {
public:
    void run(compilation::CompilationSession &session) const override {
        const auto &mir{session.artifacts().require<ir::MIRModule>("mir")};
        session.artifacts().set<std::size_t>("backend.block-count", mir.blocks.size());
    }
};

} // namespace

TEST(Extensibility, CompilationPassesUseNamedStagesAndDependencies) {
    compilation::CompilationSession session;
    session.artifacts().set<int>("value", 1);

    compilation::PassRegistry passes;
    passes.add({"second", "semantic", 0, {"first"}, {}, [](compilation::PassContext &context) {
        context.session.artifacts().require<int>("value") *= 3;
    }});
    passes.add({"first", "semantic", 0, {}, {}, [](compilation::PassContext &context) {
        context.session.artifacts().require<int>("value") += 1;
    }});

    passes.run("semantic", session);
    CHECK(session.artifacts().require<int>("value") == 6);
    CHECK(passes.order("semantic")[0] == "first");
}


TEST(Extensibility, BackendConsumesAndProducesArbitraryArtifacts) {
    compilation::CompilationSession session;
    session.artifacts().set<int>("input", 41);

    backend::BackendRegistry backends;
    backends.add("counting", std::make_shared<CountingBackend>());
    backends.run("counting", session);

    CHECK(session.artifacts().require<int>("output") == 42);
}


TEST(Extensibility, CompilationControllerProvidesOptionalClassicPipeline) {
    controllers::EngineController engine{{registry::DuplicatePolicy::Error, "root"}};
    engine.symbol("!");
    engine.node({"Root", {}, {}, {}});
    engine.parseRule("root", "!", [](parser::ParserContext &context) {
        context.consume("!");
        return ast::Node::make("Root");
    });
    controllers::CompilationController compiler{engine};
    // The compilation controller configures lowerers from the generic Engine IR layer.
    compiler.steps().lowering->hir("Root", [](const ast::Node &, ir::HIRBuilder &out, const ir::LoweringRegistry &) {
        out.emit("example.hir");
        return ir::LoweringRegistry::LoweredValue{};
    });
    compiler.steps().lowering->mir("example.hir", [](const ir::Instruction &, ir::MIRBuilder &out, ir::MIRLoweringContext &, const ir::LoweringRegistry &) {
        out.emit("example.mir");
        return ir::LoweringRegistry::LoweredValue{};
    });

    compiler.backend("classic", std::make_shared<ClassicPipelineBackend>());

    controllers::CompilationOptions options{};
    options.backend = "classic";
    compilation::CompilationSession session{compiler.compile("!", options)};

    CHECK(session.artifacts().has("ast"));
    CHECK(session.artifacts().has("hir"));
    CHECK(session.artifacts().has("mir"));
    CHECK(session.artifacts().require<ast::NodePtr>("ast")->kind() == "Root");
    CHECK(session.artifacts().require<ir::HIRModule>("hir").blocks[0].instructions[0].op == "example.hir");
    CHECK(session.artifacts().require<ir::MIRModule>("mir").blocks[0].instructions[0].op == "example.mir");
    CHECK(session.artifacts().require<std::size_t>("backend.block-count") == 1);
}


TEST(Extensibility, CompilationControllerClassicStagesCanBeDisabled) {
    controllers::EngineController engine{{registry::DuplicatePolicy::Error, "root"}};
    engine.symbol("!");
    engine.node({"Root", {}, {}, {}});
    engine.parseRule("root", "!", [](parser::ParserContext &context) {
        context.consume("!");
        return ast::Node::make("Root");
    });

    controllers::CompilationController compiler{engine};
    controllers::CompilationOptions options{};
    options.produceHIR = false;
    options.produceMIR = false;

    compilation::CompilationSession session{compiler.compile("!", options)};
    CHECK(session.artifacts().has("tokens"));
    CHECK(session.artifacts().has("ast"));
    CHECK(!session.artifacts().has("hir"));
    CHECK(!session.artifacts().has("mir"));
}


TEST(Extensibility, CompilationControllerClassicStepCanBeReplacedIndividually) {
    controllers::EngineController engine{{registry::DuplicatePolicy::Error, "root"}};
    engine.symbol("!");
    engine.node({"Root", {}, {}, {}});
    engine.parseRule("root", "!", [](parser::ParserContext &context) {
        context.consume("!");
        return ast::Node::make("Root");
    });

    controllers::CompilationController compiler{engine};
    compiler.steps().lowerHIR = [](
        const ast::Node &root,
        compilation::CompilationSession &session) {
        CHECK(root.kind() == "Root");
        session.metadata().set<std::string>("test.lowerer", "replacement");
        ir::HIRBuilder out;
        out.emit("replacement.hir");
        return out.finish();
    };

    controllers::CompilationOptions options{};
    options.produceMIR = false;
    auto session{compiler.compile("!", options)};

    CHECK(session.artifacts().require<ir::HIRModule>("hir").blocks[0].instructions[0].op == "replacement.hir");
    CHECK(session.metadata().require<std::string>("test.lowerer") == "replacement");
}


TEST(Extensibility, CompilationControllerCanUseOnlyUserProvidedSteps) {
    controllers::CompilationSteps steps{};
    steps.defaultStartDomain = [] { return std::string{"custom-root"}; };
    steps.tokenize = [](
        const std::string &text,
        std::optional<diagnostics::SourceLocation>,
        compilation::CompilationSession &session) {
        session.metadata().set<std::string>("test.source", text);
        return std::vector<token::Token>{
            {token::Kind::Symbol, "?", "", {}, 1, 1},
            {token::Kind::End, "", "", {}, 1, 2}
        };
    };
    steps.parse = [](
        std::vector<token::Token> tokens,
        const std::string &startDomain,
        compilation::CompilationSession &) {
        CHECK(startDomain == "custom-root");
        CHECK(tokens.size() == 2);
        return ast::Node::make("CustomRoot");
    };
    steps.validateAst = [](
        const ast::Node &root,
        compilation::CompilationSession &) {
        CHECK(root.kind() == "CustomRoot");
    };
    steps.lowerHIR = [](
        const ast::Node &,
        compilation::CompilationSession &) {
        ir::HIRBuilder out;
        out.emit("custom.hir");
        return out.finish();
    };
    steps.lowerMIR = [](
        const ir::HIRModule &hir,
        compilation::CompilationSession &) {
        CHECK(hir.blocks[0].instructions[0].op == "custom.hir");
        ir::MIRBuilder out;
        out.emit("custom.mir");
        return out.finish();
    };

    controllers::CompilationController compiler{std::move(steps)};
    CHECK(!compiler.hasEngine());
    bool engineAccessRejected{false};
    try {
        (void)compiler.engine();
    } catch (const std::runtime_error &) {
        engineAccessRejected = true;
    }
    CHECK(engineAccessRejected);

    auto session{compiler.compile("opaque input")};
    CHECK(session.metadata().require<std::string>("test.source") == "opaque input");
    CHECK(session.artifacts().require<ast::NodePtr>("ast")->kind() == "CustomRoot");
    CHECK(session.artifacts().require<ir::HIRModule>("hir").blocks[0].instructions[0].op == "custom.hir");
    CHECK(session.artifacts().require<ir::MIRModule>("mir").blocks[0].instructions[0].op == "custom.mir");
}


TEST(Extensibility, CompilationControllerUsesConfiguredDefaultRunOptions) {
    controllers::EngineController engine{{registry::DuplicatePolicy::Error, "root"}};
    engine.symbol("!");
    engine.node({"Root", {}, {}, {}});
    engine.parseRule("root", "!", [](parser::ParserContext &context) {
        context.consume("!");
        return ast::Node::make("Root");
    });

    controllers::CompilationControllerOptions controllerOptions{};
    controllerOptions.defaults.produceHIR = false;
    controllerOptions.defaults.produceMIR = false;

    controllers::CompilationController compiler{engine, controllerOptions};
    auto session{compiler.compile("!")};

    CHECK(session.artifacts().has("ast"));
    CHECK(!session.artifacts().has("hir"));
    CHECK(!session.artifacts().has("mir"));
}
