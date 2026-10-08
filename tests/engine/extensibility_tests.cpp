#include "../tester.hpp"
#include "NovaC.hpp"

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

TEST(Extensibility, MetadataStoreCarriesArbitraryNamedValues) {
    metadata::MetadataStore metadata;
    metadata.set<int>("example.count", 7);
    metadata.set<std::string>("example.name", "demo");

    CHECK(metadata.require<int>("example.count") == 7);
    CHECK(metadata.require<std::string>("example.name") == "demo");
    CHECK(metadata.get<double>("example.count") == nullptr);
}

TEST(Extensibility, AstSupportsOpaqueFieldsSpanAndMetadata) {
    ast::Node node{"CustomNode"};
    diagnostics::SourceSpan span{{"x.nc", 10, 2, 3}, {"x.nc", 14, 2, 7}};

    node.setOpaque<std::vector<int>>("payload", {1, 2, 3});
    node.setSpan(span);
    node.metadata().set<std::string>("example.role", "test");

    const auto *payload{node.opaque<std::vector<int>>("payload")};
    CHECK(payload != nullptr);
    CHECK(payload->size() == 3);
    CHECK(node.span().begin.offset == 10);
    CHECK(node.metadata().require<std::string>("example.role") == "test");

    ast::NodeRegistry registry;
    registry.registerNode({"CustomNode", {{"payload", ast::FieldKind::Opaque, true, {}, {}}}, {}, {}});
    registry.validate(node);
}

TEST(Extensibility, LexerCustomRulesProduceTaggedTokens) {
    lexer::LexerRegistry registry;
    registry.tokenRule("raw-name", 100, [](std::string_view remaining) -> std::optional<lexer::TokenRuleMatch> {
        if (remaining.size() < 3 || remaining.substr(0, 2) != "%%") {
            return std::nullopt;
        }
        std::size_t length{2};
        while (length < remaining.size() && remaining[length] >= 'a' && remaining[length] <= 'z') {
            ++length;
        }
        if (length == 2) {
            return std::nullopt;
        }
        return lexer::TokenRuleMatch{length, token::Kind::Identifier, std::string{remaining.substr(2, length - 2)}, {}, "$raw-name"};
    });

    lexer::Lexer lexer{registry};
    const auto tokens{lexer.tokenize("%%hello")};

    CHECK(tokens.size() == 2);
    CHECK(tokens[0].text == "hello");
    CHECK(tokens[0].tag == "$raw-name");
}

TEST(Extensibility, ParserDispatchesOnCustomTokenTag) {
    lexer::LexerRegistry lex;
    lex.tokenRule("tagged", 100, [](std::string_view remaining) -> std::optional<lexer::TokenRuleMatch> {
        if (remaining.empty() || remaining.front() != '#') {
            return std::nullopt;
        }
        return lexer::TokenRuleMatch{1, token::Kind::Symbol, "#", {}, "$tagged"};
    });
    parser::ParserRegistry registry;
    registry.rule("root", "$tagged", [](parser::ParserContext &context) {
        context.advance();
        return ast::Node::make("Tagged");
    });

    parser::Parser parser{registry, "root"};
    lexer::Lexer lexer{lex};
    CHECK(parser.parse(lexer.tokenize("#"))->kind() == "Tagged");
}

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

TEST(Extensibility, ScopeGraphAndSymbolTableResolveLexically) {
    ast::Node rootNode{"Root"};
    ast::Node childNode{"Child"};
    semantics::ScopeGraph scopes;
    const auto root{scopes.create(std::nullopt, &rootNode)};
    const auto child{scopes.create(root, &childNode)};

    semantics::SymbolTable symbols{scopes};
    const auto outer{symbols.declare("value", root, "binding", &rootNode)};
    CHECK(symbols.resolveFirst("value", child).has_value());
    CHECK(symbols.resolveFirst("value", child)->value == outer.value);
    CHECK(scopes.parentOf(child)->value == root.value);
    CHECK(scopes.scopeOf(childNode)->value == child.value);
}

TEST(Extensibility, GenericIrAcceptsRegisteredCustomOperationsAndMultipleResults) {
    ir::generic::OperationRegistry registry;
    registry.add({"example.split", 1, 1, 2, 2, {"pure"}, {}});

    ir::generic::Operation operation;
    operation.name = "example.split";
    operation.operands = {{0}};
    operation.results = {{1}, {2}};
    operation.resultTypes = {{"example.left"}, {"example.right"}};
    operation.metadata.set<std::string>("example.note", "custom");

    registry.validate(operation);
    CHECK(operation.metadata.require<std::string>("example.note") == "custom");
}

TEST(Extensibility, BackendConsumesAndProducesArbitraryArtifacts) {
    compilation::CompilationSession session;
    session.artifacts().set<int>("input", 41);

    backend::BackendRegistry backends;
    backends.add("counting", std::make_shared<CountingBackend>());
    backends.run("counting", session);

    CHECK(session.artifacts().require<int>("output") == 42);
}

TEST(Extensibility, DiagnosticsCarryLabelsNotesFixesAndMetadata) {
    diagnostics::Diagnostic diagnostic;
    diagnostic.severity = diagnostics::DiagnosticSeverity::Error;
    diagnostic.message = "invalid construct";
    diagnostic.code = "example.invalid";
    diagnostic.labels.push_back({{{"x.nc", 4, 1, 5}, {"x.nc", 5, 1, 6}}, "related location"});
    diagnostic.notes.push_back({"extra context", std::nullopt});
    diagnostic.fixes.push_back({{{"x.nc", 4, 1, 5}, {"x.nc", 5, 1, 6}}, "replacement", "use this form"});
    diagnostic.metadata.set<int>("example.rank", 3);

    diagnostics::DiagnosticEngine engine;
    engine.report(std::move(diagnostic));

    CHECK(engine.diagnostics()[0].code == "example.invalid");
    CHECK(engine.diagnostics()[0].metadata.require<int>("example.rank") == 3);
    CHECK(engine.format().find("extra context") != std::string::npos);
    CHECK(engine.format().find("replacement") != std::string::npos);
}


TEST(Extensibility, CompilationControllerProvidesOptionalClassicPipeline) {
    controllers::EngineController engine{{registry::DuplicatePolicy::Error, "root"}};
    engine.symbol("!");
    engine.node({"Root", {}, {}, {}});
    engine.parseRule("root", "!", [](parser::ParserContext &context) {
        context.consume("!");
        return ast::Node::make("Root");
    });
    engine.hir("Root", [](const ast::Node &, ir::HIRBuilder &out, const ir::LoweringRegistry &) {
        out.emit("example.hir");
        return ir::LoweringRegistry::LoweredValue{};
    });
    engine.mir("example.hir", [](const ir::Instruction &, ir::MIRBuilder &out, ir::MIRLoweringContext &, const ir::LoweringRegistry &) {
        out.emit("example.mir");
        return ir::LoweringRegistry::LoweredValue{};
    });

    controllers::CompilationController compiler{engine};
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
