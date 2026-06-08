#include "include/compiler/Compiler.hpp"
#include "include/compiler/Pass.hpp"
#include "include/language/Language.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

using namespace novac;

class DumpMIRPass final : public compiler::Pass {
public:
    std::string name() const override
    {
        return "dump.mir";
    }

    void run(compiler::CompilationContext &context) const override
    {
        const ir::MIRModule &module{
            context.requireArtifact<ir::MIRModule>("mir")
        };

        std::cout << "\n=== MIR ===\n";

        for (const ir::MIRNode &node : module.nodes) {
            std::cout << node.op;

            for (const std::string &operand : node.operands) {
                std::cout << " " << operand;
            }

            std::cout << '\n';
        }
    }
};

int main()
{
    try {
        language::Language language{};

        language.lexer.symbol("+");

        language.nodes.registerNode({
            "program",
            {{"expression", ast::FieldKind::Node}},
            "program root"
        });

        language.nodes.registerNode({
            "integer",
            {{"value", ast::FieldKind::Int}},
            "integer literal"
        });

        language.nodes.registerNode({
            "binary",
            {
                {"left", ast::FieldKind::Node},
                {"right", ast::FieldKind::Node},
                {"op", ast::FieldKind::String}
            },
            "binary operation"
        });

        language.parser.prefix(
            "expr",
            "$int",
            [](parser::ParserContext &context) {
                const auto token{context.consumeKind(token::Kind::Integer)};

                ast::NodePtr node{ast::Node::make("integer")};
                node->set("value", std::stoi(token.text));

                return node;
            });

        language.parser.infix(
            "expr",
            "+",
            10,
            [](parser::ParserContext &context, ast::NodePtr left) {
                context.consume("+");

                ast::NodePtr right{context.parse("expr", 11)};
                ast::NodePtr node{ast::Node::make("binary")};

                node->set("left", left);
                node->set("right", right);
                node->set("op", std::string{"+"});

                return node;
            });

        language.parser.fallback(
            "program",
            [](parser::ParserContext &context) {
                ast::NodePtr expression{context.parse("expr")};

                ast::NodePtr program{ast::Node::make("program")};
                program->set("expression", expression);

                return program;
            });

        language.runtime.expression(
            "integer",
            [](const ast::Node &node, const runtime::RuntimeContext &) {
                return runtime::Value::integer(node.integer("value"));
            });

        language.runtime.binaryOperator(
            "+",
            [](const ast::Node &node, const runtime::RuntimeContext &context) {
                const int lhs{context.eval(*node.child("left")).asInt()};
                const int rhs{context.eval(*node.child("right")).asInt()};

                return runtime::Value::integer(lhs + rhs);
            });

        language.runtime.expression(
            "program",
            [](const ast::Node &node, const runtime::RuntimeContext &context) {
                return context.eval(*node.child("expression"));
            });

        language.lowering.hir(
            "integer",
            [](const ast::Node &node, ir::HIRBuilder &builder, const ir::LoweringRegistry &) {
                builder.emit("hir.const", {std::to_string(node.integer("value"))});
            });

        language.lowering.hir(
            "binary",
            [](const ast::Node &node, ir::HIRBuilder &builder, const ir::LoweringRegistry &registry) {
                registry.lowerHIR(*node.child("left"), builder);
                registry.lowerHIR(*node.child("right"), builder);

                builder.emit("hir.add");
            });

        language.lowering.hir("program", [](const ast::Node &node, ir::HIRBuilder &builder, const ir::LoweringRegistry &registry) { registry.lowerHIR(*node.child("expression"), builder); });
        language.lowering.mir("hir.const", [](const ir::HIRNode &node, ir::MIRBuilder &builder, const ir::LoweringRegistry &) { builder.emit("mir.const", node.operands); });
        language.lowering.mir("hir.add", [](const ir::HIRNode &, ir::MIRBuilder &builder, const ir::LoweringRegistry &) { builder.emit("mir.add"); });

        compiler::Compiler compiler{language, "program"};

        compiler.addPass<compiler::ParsePass>("ast");
        compiler.addPass<compiler::AstValidationPass>("ast");
        compiler.addPass<compiler::HIRLoweringPass>("ast", "hir");
        compiler.addPass<compiler::MIRLoweringPass>("hir", "mir");
        compiler.addPass<DumpMIRPass>();
        compiler.addPass<compiler::RuntimePass>("ast", "result");

        const std::string source{"40 + 2 + 8"};

        compiler::CompilationContext context{compiler.run(source)};

        const runtime::Value &result{
            context.requireArtifact<runtime::Value>("result")
        };

        std::cout << "\nResult = " << result.toString() << '\n';

        return 0;
    } catch (const std::exception &exception) {
        std::cerr << exception.what() << '\n';

        return 1;
    }
}