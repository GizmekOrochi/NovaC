#include "include/compiler/Compiler.hpp"
#include "include/language/Language.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace novac;

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

        language.lowering.hir(
            "program",
            [](const ast::Node &node, ir::HIRBuilder &builder, const ir::LoweringRegistry &registry) {
                registry.lowerHIR(*node.child("expression"), builder);
            });

        language.lowering.mir(
            "hir.const",
            [](const ir::HIRNode &node, ir::MIRBuilder &builder, const ir::LoweringRegistry &) {
                builder.emit("mir.const", node.operands);
            });

        language.lowering.mir(
            "hir.add",
            [](const ir::HIRNode &, ir::MIRBuilder &builder, const ir::LoweringRegistry &) {
                builder.emit("mir.add");
            });

        class DumpBackend final : public backend::Backend {
        public:
            std::string name() const override
            {
                return "dump";
            }

            void emit(const ir::MIRModule &module) override
            {
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

        language.backends.add(
            "dump",
            [] {
                return std::make_unique<DumpBackend>();
            });

        compiler::Compiler compiler{language, "program"};

        const std::string source{"40 + 2 + 8"};

        const ast::NodePtr root{compiler.parse(source)};
        const ir::HIRModule hir{compiler.lowerToHIR(*root)};
        const ir::MIRModule mir{compiler.lowerToMIR(*root)};
        const runtime::Value result{compiler.run(source)};

        static_cast<void>(hir);
        static_cast<void>(mir);

        compiler.emit(*root, "dump");

        std::cout << "\nResult = " << result.toString() << '\n';

        return 0;
    } catch (const std::exception &exception) {
        std::cerr << exception.what() << '\n';

        return 1;
    }
}