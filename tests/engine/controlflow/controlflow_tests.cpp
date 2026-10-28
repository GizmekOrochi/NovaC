#include "../../tester.hpp"
#include "novac/engine/controlflow/ControlFlow.hpp"

#include <stdexcept>
#include <string>
#include <variant>

namespace {

using novac::ast::FieldKind;
using novac::ast::Node;
using novac::ast::NodePtr;
using novac::controllers::EngineController;
using novac::controllers::EngineFeature;
using novac::ids::ParseDomain;
using novac::runtime::Value;

EngineController makeMinimalEngine(int &hits) {
    EngineController engine{{.startDomain = "statement"}};

    engine.keyword("false");
    engine.node({"Bool", {{"value", FieldKind::Bool, true}}, {"Expression"}, {}});
    engine.node({"Hit", {}, {"Statement"}, {}});

    engine.parseRule("expression", "false", [](novac::parser::ParserContext &context) {
        context.consume("false");
        NodePtr node{Node::make("Bool")};
        node->set("value", false);
        return node;
    });
    engine.parseRule("statement", "$identifier", [](novac::parser::ParserContext &context) {
        context.advance();
        return Node::make("Hit");
    });

    engine.expression("Bool", [](const Node &node, novac::runtime::RuntimeContext &) {
        return Value::boolean(std::get<bool>(node.field("value")));
    });
    engine.statement("Hit", [&hits](const Node &, novac::runtime::RuntimeContext &) {
        ++hits;
    });

    return engine;
}

EngineFeature unlessFeature() {
    EngineFeature feature{"test.unless"};
    feature.onInstall(novac::controlflow::statement({
        .domain = ParseDomain{"statement"},
        .trigger = "unless",
        .triggerRegistration = novac::controlflow::TriggerRegistration::Keyword,
        .schema = {
            "Unless",
            {
                {"condition", FieldKind::Node, true, {}, {"Expression"}},
                {"body", FieldKind::Node, true, {}, {"Statement"}}
            },
            {"Statement"},
            "Test-only custom conditional"
        },
        .parse = [](novac::parser::ParserContext &context) {
            context.consume("unless");
            NodePtr condition{context.parse("expression")};
            NodePtr body{context.parse("statement")};
            NodePtr node{Node::make("Unless")};
            node->set("condition", condition);
            node->set("body", body);
            return node;
        },
        .runtime = novac::runtime::StmtHandler{
            [](const Node &node, novac::runtime::RuntimeContext &context) {
                if (!context.eval(*node.child("condition")).truthy()) {
                    context.exec(*node.child("body"));
                }
            }}
    }));
    return feature;
}

} // namespace

TEST(ControlFlow, CustomStatementUsesRegularEngineFeaturePipeline) {
    int hits{};
    EngineController engine{makeMinimalEngine(hits)};
    engine.install(unlessFeature());

    NodePtr root{engine.parse("unless false hit")};
    CHECK(root->kind() == "Unless");
    engine.validate(*root);

    engine.exec(*root);
    CHECK_EQ(hits, 1);
}

TEST(ControlFlow, LoopGuardEnforcesMaximum) {
    novac::controlflow::LoopGuard guard{2};
    guard.step();
    guard.step();
    CHECK_EQ(guard.iterations(), std::size_t{2});

    bool threw{};
    try {
        guard.step();
    } catch (const std::runtime_error &) {
        threw = true;
    }
    CHECK(threw);
}

TEST(ControlFlow, UnlimitedLoopGuardDoesNotRejectIterations) {
    novac::controlflow::LoopGuard guard{};
    for (int index{}; index < 100; ++index) {
        guard.step();
    }
    CHECK_EQ(guard.iterations(), std::size_t{100});
}

TEST(ControlFlow, FailedFeatureRollsBackDirectiveAndStatementRegistrations) {
    EngineController engine{{.startDomain = "statement"}};

    const auto makeStatementInstaller = [] {
        return novac::controlflow::statement({
            .domain = ParseDomain{"statement"},
            .trigger = "guard",
            .triggerRegistration = novac::controlflow::TriggerRegistration::Keyword,
            .schema = {"Guard", {}, {"Statement"}, "Rollback test statement"},
            .parse = [](novac::parser::ParserContext &context) {
                context.consume("guard");
                return Node::make("Guard");
            },
            .runtime = novac::runtime::StmtHandler{
                [](const Node &, novac::runtime::RuntimeContext &) {}}
        });
    };

    EngineFeature failing{"test.rollback.failing"};
    failing
        .onInstall([](EngineController &candidate) {
            candidate.directive(
                "rollback",
                [](const novac::source::Directive &, novac::source::PreprocessorContext &) {});
        })
        .onInstall(makeStatementInstaller())
        .onInstall([](EngineController &) {
            throw std::runtime_error("intentional install failure");
        });

    bool threw{};
    try {
        engine.install(failing);
    } catch (const std::runtime_error &) {
        threw = true;
    }
    CHECK(threw);
    CHECK(!engine.hasFeature("test.rollback.failing"));

    EngineFeature replacement{"test.rollback.replacement"};
    replacement
        .onInstall([](EngineController &candidate) {
            candidate.directive(
                "rollback",
                [](const novac::source::Directive &, novac::source::PreprocessorContext &) {});
        })
        .onInstall(makeStatementInstaller());

    engine.install(replacement);
    CHECK(engine.hasFeature("test.rollback.replacement"));
    CHECK(engine.lexer().isKeyword("guard"));
}
