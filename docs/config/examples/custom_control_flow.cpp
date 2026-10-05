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
            }},
        .hir = novac::ir::LoweringRegistry::HIRLowerer{
            [](const Node &node,
               novac::ir::HIRBuilder &out,
               const novac::ir::LoweringRegistry &lowering) {
                const auto condition{lowering.lowerHIR(*node.child("condition"), out)};
                if (!condition) {
                    throw std::runtime_error("unless condition did not produce a HIR value");
                }

                const auto body{out.createBlock("unless.body")};
                const auto exit{out.createBlock("unless.exit")};

                out.terminate(
                    "branch.false",
                    {novac::ir::Operand::fromValue(*condition)},
                    {body, exit});

                out.setCurrentBlock(body);
                lowering.lowerHIR(*node.child("body"), out);
                if (!out.currentBlockTerminated()) {
                    out.terminate("jump", {}, {exit});
                }

                out.setCurrentBlock(exit);
                return novac::ir::LoweringRegistry::LoweredValue{};
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

    engine.hir("Bool", [](const Node &node,
                          novac::ir::HIRBuilder &out,
                          const novac::ir::LoweringRegistry &) {
        return novac::ir::LoweringRegistry::LoweredValue{
            out.emitValue(
                "const.bool",
                {novac::ir::Operand::fromLiteral(std::get<bool>(node.field("value")))},
                novac::ir::ValueType::Bool)};
    });
    engine.hir("Hit", [](const Node &,
                         novac::ir::HIRBuilder &out,
                         const novac::ir::LoweringRegistry &) {
        out.emit("hit");
        return novac::ir::LoweringRegistry::LoweredValue{};
    });

    // The custom feature uses only Engine APIs; Essentials is not installed.
    engine.install(makeUnlessFeature());

    const NodePtr program{engine.parse("unless false hit")};
    engine.validate(*program);
    engine.exec(*program);

    const novac::ir::HIRModule hir{engine.lowerToHIR(*program)};

    std::cout << "runtime hits=" << hits << '\n';
    std::cout << "hir blocks=" << hir.blocks.size() << '\n';
    std::cout << hir.blocks[0].name << " -> " << hir.blocks[0].terminator->op << '\n';
    std::cout << hir.blocks[1].name << " -> " << hir.blocks[1].terminator->op << '\n';
    std::cout << hir.blocks[2].name << " -> open\n";
}
