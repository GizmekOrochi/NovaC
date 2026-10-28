// Opt-in only; this tests the new compilation asset separately from Engine.
#include "../../tester.hpp"
#include "novac/assets/compilation/ModularCompilationController.hpp"
#include "novac/assets/compilation/backend/CSourceBackend.hpp"
#include "novac/assets/compilation/backend/X86_64Backend.hpp"
#include <vector>

#include <memory>
#include <stdexcept>
#include <string>

using namespace novac;

namespace {
using Comp = compilation::ModularCompilationController;
using Ctx = compilation::FeatureLoweringContext;
using Facet = compilation::CompilationFacet;
using Resolved = compilation::ResolvedOperation;

ast::NodePtr literal(int number) {
    auto node = ast::Node::make("Integer");
    node->set("value", number);
    return node;
}
ast::NodePtr binary(const std::string &op, ast::NodePtr a, ast::NodePtr b) {
    auto node = ast::Node::make("BinaryExpr");
    node->set("op", op).set("left", std::move(a)).set("right", std::move(b));
    return node;
}
ast::NodePtr program(ast::NodePtr body) {
    auto node = ast::Node::make("Program");
    node->set("body", std::move(body));
    return node;
}

compilation::SemanticResolver resolve = [](const ast::Node &node) -> Resolved {
    if (node.kind() == "Integer") return {"literal.i32", ir::TypeRef{"i32", "novac.prototype"}};
    if (node.kind() == "Program") return {"program.return", ir::ValueType::Void};
    if (node.kind() == "BinaryExpr") {
        if (node.str("op") == "+") return {"number.add.i32", ir::TypeRef{"i32", "novac.prototype"}};
        if (node.str("op") == "*") return {"number.mul.i32", ir::TypeRef{"i32", "novac.prototype"}};
    }
    throw std::runtime_error("unresolved semantic operation");
};

void installCore(Comp &compiler) {
    compiler.features().attach(Facet{"language.entry", "Program", "program.return",
        [](const ast::Node &node, Ctx &ctx) -> std::optional<ir::ValueId> {
            auto v = ctx.requireValue(*node.child("body"));
            ctx.builder().terminate("return", {ir::Operand::fromValue(v)});
            return {};
        }});
    compiler.features().attach(Facet{"atomic.integer", "Integer", "literal.i32",
        [](const ast::Node &node, Ctx &ctx) -> std::optional<ir::ValueId> {
            return ctx.builder().emitValue("novac.i32.const", {ir::Operand::fromLiteral(node.integer("value"))},
                                           ctx.semantic().type);
        }});
    compiler.features().attach(Facet{"atomic.add", "BinaryExpr", "number.add.i32",
        [](const ast::Node &node, Ctx &ctx) -> std::optional<ir::ValueId> {
            auto left = ctx.requireValue(*node.child("left"));
            auto right = ctx.requireValue(*node.child("right"));
            return ctx.builder().emitValue("novac.i32.add",
                {ir::Operand::fromValue(left), ir::Operand::fromValue(right)}, ctx.semantic().type);
        }, {"atomic.integer"}, ir::TypeRef{ir::TypeRef{"i32", "novac.prototype"}}});
    compiler.lowering().mir("novac.i32.const", [](const ir::MIRLoweringInput &in, ir::MIRBuilder &b,
                          const ir::MIRLoweringContext &, const ir::LoweringRegistry &) -> ir::MIRLoweringResult {
        return b.emitValue(in.source.op, in.operands, in.source.type);
    });
    compiler.lowering().mir("novac.i32.add", [](const ir::MIRLoweringInput &in, ir::MIRBuilder &b,
                          const ir::MIRLoweringContext &, const ir::LoweringRegistry &) -> ir::MIRLoweringResult {
        return b.emitValue(in.source.op, in.operands, in.source.type);
    });
}

template <typename F> std::string errorOf(F &&fn) {
    try { fn(); } catch (const std::runtime_error &e) {return e.what();}
    return {};
}
}

TEST(ModularCompilation, TwoIndependentFeaturesShareBinaryExpr) {
    Comp compiler;
    installCore(compiler);
    compiler.features().attach(Facet{"atomic.mul", "BinaryExpr", "number.mul.i32",
        [](const ast::Node &node, Ctx &ctx) -> std::optional<ir::ValueId> {
            auto left = ctx.requireValue(*node.child("left"));
            auto right = ctx.requireValue(*node.child("right"));
            return ctx.builder().emitValue("novac.i32.mul", {ir::Operand::fromValue(left), ir::Operand::fromValue(right)},
                                          ctx.semantic().type);
        }});
    compiler.lowering().mir("novac.i32.mul", [](const ir::MIRLoweringInput &in, ir::MIRBuilder &b,
                           const ir::MIRLoweringContext &, const ir::LoweringRegistry &) -> ir::MIRLoweringResult {
        return b.emitValue(in.source.op, in.operands, in.source.type);
    });
    auto root = program(binary("*", binary("+", literal(1), literal(2)), literal(3)));
    auto session = compiler.compile(*root, resolve);
    auto &mir = session.artifacts().require<ir::MIRModule>("mir");
    CHECK(mir.blocks[0].instructions.size() == 5);
    CHECK(mir.blocks[0].instructions.back().op == "novac.i32.mul");
}

TEST(ModularCompilation, EndToEndProducesCForFortyTwo) {
    Comp compiler; installCore(compiler);
    compiler.backends().add("c", std::make_shared<backend::CSourceBackend>());
    auto root = program(binary("+", literal(40), literal(2)));
    auto session = compiler.compile(*root, resolve, "c");
    CHECK(session.artifacts().has("hir"));
    CHECK(session.artifacts().has("mir"));
    const auto &c = session.artifacts().require<std::string>("c-source");
    CHECK(c.find("uint32_t v2 = v0 + v1") != std::string::npos);
    CHECK(c.find("return (int)(v2 & 255u)") != std::string::npos);
    CHECK(c.find("40u") != std::string::npos);
}

TEST(ModularCompilation, MissingBackendCapabilityCannotSilentlyFallback) {
    Comp compiler; installCore(compiler);
    compiler.features().attach(Facet{"atomic.mul", "BinaryExpr", "number.mul.i32",
        [](const ast::Node &node, Ctx &ctx) -> std::optional<ir::ValueId> {
            auto left = ctx.requireValue(*node.child("left"));
            auto right = ctx.requireValue(*node.child("right"));
            return ctx.builder().emitValue("novac.i32.mul", {ir::Operand::fromValue(left), ir::Operand::fromValue(right)},
                                           ir::TypeRef{"i32", "novac.prototype"});
        }});
    compiler.lowering().mir("novac.i32.mul", [](const ir::MIRLoweringInput &in, ir::MIRBuilder &b,
                           const ir::MIRLoweringContext &, const ir::LoweringRegistry &) -> ir::MIRLoweringResult {
        return b.emitValue(in.source.op, in.operands, in.source.type);
    });
    compiler.backends().add("c", std::make_shared<backend::CSourceBackend>());
    auto root = program(binary("*", literal(3), literal(4)));
    auto message = errorOf([&] { compiler.compile(*root, resolve, "c"); });
    CHECK(message.find("cannot compile MIR operation 'novac.i32.mul'") != std::string::npos);
}

TEST(ModularCompilation, MissingFeatureReportsExactOperation) {
    Comp compiler; installCore(compiler);
    auto root = program(binary("*", literal(3), literal(4)));
    auto message = errorOf([&] { compiler.compile(*root, resolve); });
    CHECK(message.find("number.mul.i32") != std::string::npos);
}

TEST(ModularCompilation, DuplicatesAndDependenciesFailBeforeLowering) {
    Comp compiler; installCore(compiler);
    CHECK(errorOf([&] {
        compiler.features().attach(Facet{"other.feature", "BinaryExpr", "number.add.i32",
            [](const ast::Node &, Ctx &) -> std::optional<ir::ValueId> { return {}; }});
    }).find("ambiguous handler") != std::string::npos);
    Comp incomplete;
    incomplete.features().attach(Facet{"dependent", "Program", "program.return",
        [](const ast::Node &, Ctx &) -> std::optional<ir::ValueId> {return {};}, {"missing"}});
    auto root = program(literal(5));
    CHECK(errorOf([&] { incomplete.compile(*root, resolve); }).find("missing feature") != std::string::npos);
}

TEST(ModularCompilation, ArtifactInsertNeverOverwritesAnotherProducer) {
    compilation::ArtifactStore store;
    store.insert<int>("result", 42);
    CHECK(errorOf([&] { store.insert<int>("result", 1); }).find("duplicate artifact") != std::string::npos);
    CHECK(store.require<int>("result") == 42);
}

TEST(ModularCompilation, NodeKindAloneNeverSelectsWrongSemantics) {
    Comp compiler; installCore(compiler);
    auto root = program(binary("+", literal(1), literal(2)));
    compilation::SemanticResolver wrong = [](const ast::Node &node) -> Resolved {
        if (node.kind() == "BinaryExpr") return {"unrelated.subtract", ir::TypeRef{"i32", "novac.prototype"}};
        return resolve(node);
    };
    CHECK(errorOf([&] { compiler.compile(*root, wrong); }).find("unrelated.subtract") != std::string::npos);
}

TEST(ModularCompilation, RepeatedCompilationsHaveIsolatedSessions) {
    Comp compiler; installCore(compiler);
    auto a = program(binary("+", literal(1), literal(2)));
    auto b = program(binary("+", literal(5), literal(6)));
    auto first = compiler.compile(*a, resolve);
    auto second = compiler.compile(*b, resolve);
    auto &left = first.artifacts().require<ir::HIRModule>("hir");
    auto &right = second.artifacts().require<ir::HIRModule>("hir");
    CHECK(left.blocks[0].instructions[0].operands[0].toString() !=
          right.blocks[0].instructions[0].operands[0].toString());
}

TEST(ModularCompilation, RejectsCyclesInASTInsteadOfInfiniteRecursion) {
    Comp compiler; installCore(compiler);
    auto root = program(literal(10));
    root->set("body", root);
    CHECK(errorOf([&] { compiler.compile(*root, resolve); }).find("cyclic") != std::string::npos);
    root->set("body", literal(10));
}

TEST(ModularCompilation, SemanticTypeIsNotJustAnUnverifiedHint) {
    Comp compiler; installCore(compiler);
    compilation::SemanticResolver badType = [](const ast::Node &node) -> Resolved {
        if (node.kind() == "BinaryExpr") return {"number.add.i32", ir::ValueType::Bool};
        return resolve(node);
    };
    auto root = program(binary("+", literal(1), literal(2)));
    CHECK(errorOf([&] { compiler.compile(*root, badType); }).find("type/identity mismatch") != std::string::npos);
}

TEST(ModularCompilation, DetectsCyclicFeatureDependencies) {
    Comp compiler;
    compiler.features().attach(Facet{"one", "A", "A",
        [](const ast::Node &, Ctx &) -> std::optional<ir::ValueId> { return {}; }, {"two"}});
    compiler.features().attach(Facet{"two", "B", "B",
        [](const ast::Node &, Ctx &) -> std::optional<ir::ValueId> { return {}; }, {"one"}});
    CHECK(errorOf([&] { compiler.features().validate(); }).find("cyclic feature") != std::string::npos);
}

TEST(ModularCompilation, BackendCannotPretendItProducedTheOutput) {
    class FakeBackend final : public backend::Backend {
    public:
        std::optional<backend::BackendCapabilities> capabilities() const override {
            return backend::BackendCapabilities{"mir", "fake-output", {"novac.i32.const", "novac.i32.add"}, {"return"}};
        }
        void run(compilation::CompilationSession &) const override {}
    };
    Comp compiler; installCore(compiler);
    compiler.backends().add("fake", std::make_shared<FakeBackend>());
    auto root = program(literal(42));
    CHECK(errorOf([&] { compiler.compile(*root, resolve, "fake"); }).find("failed to produce") != std::string::npos);
}

TEST(ModularCompilation, BackendLegacyWithoutCapabilityContractIsRejected) {
    class LegacyBackend final : public backend::Backend {
    public: void run(compilation::CompilationSession &) const override {}
    };
    Comp compiler; installCore(compiler);
    compiler.backends().add("legacy", std::make_shared<LegacyBackend>());
    auto root = program(literal(42));
    CHECK(errorOf([&] { compiler.compile(*root, resolve, "legacy"); }).find("no valid capabilities") != std::string::npos);
}

TEST(ModularCompilation, LazyFeatureControlsItsOwnChildEvaluation) {
    Comp compiler; installCore(compiler);
    compiler.features().attach(Facet{"literal.bool", "Boolean", "literal.bool",
        [](const ast::Node &node, Ctx &ctx) -> std::optional<ir::ValueId> {
            return ctx.builder().emitValue("novac.bool.const",
                {ir::Operand::fromLiteral(std::get<bool>(node.field("value")))}, ir::ValueType::Bool);
        }, {}, ir::TypeRef{ir::ValueType::Bool}});
    compiler.features().attach(Facet{"logical.and", "LazyAnd", "logical.lazy.and",
        [](const ast::Node &node, Ctx &ctx) -> std::optional<ir::ValueId> {
            // Safe language-specific constant short-circuit: right node is
            // deliberately never lowered when left is constant false.
            const auto left = node.child("left");
            const auto leftValue = ctx.requireValue(*left);
            if (!std::get<bool>(left->field("value"))) return leftValue;
            return ctx.requireValue(*node.child("right"));
        }, {"literal.bool"}, ir::TypeRef{ir::ValueType::Bool}});
    compiler.lowering().mir("novac.bool.const", [](const ir::MIRLoweringInput &in, ir::MIRBuilder &b,
                           const ir::MIRLoweringContext &, const ir::LoweringRegistry &) -> ir::MIRLoweringResult {
        return b.emitValue(in.source.op, in.operands, in.source.type);
    });
    auto constant = ast::Node::make("Boolean"); constant->set("value", false);
    auto neverRun = ast::Node::make("DangerousEffect");
    auto expr = ast::Node::make("LazyAnd"); expr->set("left", constant).set("right", neverRun);
    auto root = program(expr);
    compilation::SemanticResolver semantic = [](const ast::Node &node) -> Resolved {
        if (node.kind() == "Boolean") return {"literal.bool", ir::ValueType::Bool};
        if (node.kind() == "LazyAnd") return {"logical.lazy.and", ir::ValueType::Bool};
        if (node.kind() == "Program") return {"program.return", ir::ValueType::Void};
        throw std::runtime_error("dangerous child unexpectedly visited");
    };
    const auto session = compiler.compile(*root, semantic);
    CHECK(session.artifacts().require<ir::HIRModule>("hir").blocks[0].instructions.size() == 1);
}

TEST(ModularCompilation, TwoBackendsCanConsumeTheSamePortableContributions) {
    class TextBackend final : public backend::Backend {
    public:
        std::optional<backend::BackendCapabilities> capabilities() const override {
            return backend::BackendCapabilities{"mir", "text-output", {"novac.i32.const", "novac.i32.add"}, {"return"}};
        }
        void run(compilation::CompilationSession &session) const override {
            session.artifacts().insert<std::string>("text-output", "mir-instructions=" +
                std::to_string(session.artifacts().require<ir::MIRModule>("mir").blocks[0].instructions.size()));
        }
    };
    Comp compiler; installCore(compiler);
    compiler.backends().add("text", std::make_shared<TextBackend>());
    compiler.backends().add("c", std::make_shared<backend::CSourceBackend>());
    auto root = program(binary("+", literal(40), literal(2)));
    const auto a = compiler.compile(*root, resolve, "text");
    const auto b = compiler.compile(*root, resolve, "c");
    CHECK(a.artifacts().require<std::string>("text-output") == "mir-instructions=3");
    CHECK(b.artifacts().require<std::string>("c-source").find("v0 + v1") != std::string::npos);
}

TEST(ModularCompilation, NativeTargetResolvedByArchitectureOSABIAndFormat) {
    Comp compiler; installCore(compiler);
    compiler.targets().add("linux-x64",backend::makeX86_64LinuxTarget());
    auto root=program(binary("+",literal(40),literal(2)));
    const auto target=compilation::target::linuxX86_64();
    const auto session=compiler.compileForTarget(*root,resolve,target);
    CHECK(session.artifacts().has("mir"));
    CHECK(session.artifacts().has("x86-64-elf-executable"));
    const auto &elf=session.artifacts().require<std::vector<std::uint8_t>>("x86-64-elf-executable");
    CHECK(elf.size()>4096 && elf[18]==62);
    auto unsupported=target;unsupported.operatingSystem="macos";unsupported.objectFormat="mach-o";
    unsupported.abi="apple-arm64";
    CHECK(errorOf([&]{compiler.compileForTarget(*root,resolve,unsupported);}).find("unsupported target")!=std::string::npos);
}

TEST(ModularCompilation, RegistrySelectedNativeBackendRunsThroughController) {
    Comp compiler;
    installCore(compiler);
    compiler.targets().add("linux-x86-64", backend::makeX86_64LinuxTarget());
    auto root = program(binary("+", literal(40), literal(2)));
    auto session = compiler.compileForTarget(*root, resolve, compilation::target::linuxX86_64());
    CHECK(session.artifacts().has("hir"));
    CHECK(session.artifacts().has("mir"));
    CHECK(session.artifacts().has("x86-64-machine-code"));
    const auto &elf = session.artifacts().require<std::vector<std::uint8_t>>("x86-64-elf-executable");
    CHECK(elf.size() > 4096);
    CHECK(elf[0] == 0x7F && elf[1] == 'E' && elf[2] == 'L' && elf[3] == 'F');
    auto unsupported = compilation::target::linuxX86_64();
    unsupported.operatingSystem = "windows";
    unsupported.abi = "win64";
    unsupported.objectFormat = "coff";
    CHECK(errorOf([&] { compiler.compileForTarget(*root, resolve, unsupported); })
              .find("unsupported target") != std::string::npos);
}

TEST(ModularCompilation, ExplicitProviderSelectionWithOptionsIsSupported) {
    Comp compiler;
    installCore(compiler);
    compiler.targets().add("standard",backend::makeX86_64LinuxTarget());
    compiler.targets().add("alternate",backend::makeX86_64LinuxTarget());
    auto root=program(binary("+",literal(40),literal(2)));
    const auto spec=compilation::target::linuxX86_64();
    CHECK(errorOf([&]{ (void)compiler.compileForTarget(*root,resolve,spec); }).find("ambiguous target") != std::string::npos);
    const auto result=compiler.compileForTarget(*root,resolve,spec,
       compilation::CompilationOptions{compilation::machine::OptimizationGoal::Size},"alternate");
    CHECK(result.artifacts().has("x86-64-elf-executable"));
}

namespace {
class GoalAwareX86Target final : public backend::LinuxX86_64Target {
public:
    mutable std::vector<compilation::machine::OptimizationGoal> seenGoals;
protected:
    backend::NativeBuild generateForTarget(const ir::MIRModule &mir,
                         const compilation::machine::MachineTarget &target) const override {
        seenGoals.push_back(target.goal);
        return backend::LinuxX86_64Target::generateForTarget(mir, target);
    }
};
}
TEST(ModularCompilation, TargetProviderInheritanceReceivesOptionsFromController) {
    Comp compiler;
    installCore(compiler);
    auto native = std::make_shared<GoalAwareX86Target>();
    compiler.targets().add("derived", native);
    auto root = program(binary("+", literal(40), literal(2)));
    for (const auto goal : {compilation::machine::OptimizationGoal::Debug,
                            compilation::machine::OptimizationGoal::Size,
                            compilation::machine::OptimizationGoal::Speed}) {
        const auto session = compiler.compileForTarget(*root, resolve,
            compilation::target::linuxX86_64(), compilation::CompilationOptions{goal}, "derived");
        CHECK(session.artifacts().has("x86-64-elf-executable"));
        CHECK(native->seenGoals.back() == goal);
    }
    CHECK(native->seenGoals.size() == 3);
}
