#include <iostream>
#include <stdexcept>
#include <variant>

#include <NovaC.hpp>

namespace {

using novac::ast::FieldKind;
using novac::ast::Node;
using novac::ast::NodePtr;
using novac::controllers::EngineController;
using novac::controllers::EngineFeature;
using novac::ids::ParseDomain;
using novac::runtime::Value;

EngineFeature makeUnlessFeature() {
    EngineFeature feature{"example.unless"};

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
            "Language-defined conditional statement"
        },
        .parse = [](novac::parser::ParserContext &context) {
            context.consume("unless");

            NodePtr node{Node::make("Unless")};
            node->set("condition", context.parse("expression"));
            node->set("body", context.parse("statement"));
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

int main() {
    int hits{};
    EngineController engine{{.startDomain = "statement"}};

    // Minimal host language used by the custom statement.
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

    // The custom feature uses only Engine APIs; Essentials is not installed.
    engine.install(makeUnlessFeature());

    const std::string source{R"(unless false hit)"};
    const NodePtr program{engine.parse(source)};
    engine.validate(*program);
    engine.exec(*program);
    std::cout << "runtime hits=" << hits << '\n';
}
