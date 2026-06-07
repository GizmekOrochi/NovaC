#include "include/compiler/Compiler.hpp"
#include "include/language/Language.hpp"

#include <iostream>

using namespace novac;

int main()
{
    try {
        language::Language language{};

        //
        // TOKENS
        //

        language.lexer.symbol("+");

        //
        // AST
        //

        language.nodes.registerNode({
            "program",
            {
                {"expression", ast::FieldKind::Node}
            },
            "program root"
        });

        language.nodes.registerNode({
            "integer",
            {
                {"value", ast::FieldKind::Int}
            },
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

        //
        // PARSER
        //

        language.parser.prefix(
            "expr",
            "$int",
            [](parser::ParserContext &context)
            {
                const auto token{
                    context.consumeKind(token::Kind::Integer)
                };

                auto node{
                    ast::Node::make("integer")
                };

                node->set(
                    "value",
                    std::stoi(token.text));

                return node;
            });

        language.parser.infix(
            "expr",
            "+",
            10,
            [](parser::ParserContext &context,
               ast::NodePtr left)
            {
                context.consume("+");

                auto right{
                    context.parse(
                        "expr",
                        11)
                };

                auto node{
                    ast::Node::make("binary")
                };

                node->set("left", left);
                node->set("right", right);
                node->set("op", std::string{"+"});

                return node;
            });

        language.parser.fallback(
            "program",
            [](parser::ParserContext &context)
            {
                auto expr{
                    context.parse("expr")
                };

                auto program{
                    ast::Node::make("program")
                };

                program->set(
                    "expression",
                    expr);

                return program;
            });

        //
        // RUNTIME
        //

        language.runtime.expression(
            "integer",
            [](const ast::Node &node,
               const runtime::RuntimeContext &)
            {
                return runtime::Value::integer(
                    node.integer("value"));
            });

        language.runtime.binaryOperator(
            "+",
            [](const ast::Node &node,
               const runtime::RuntimeContext &context)
            {
                const int lhs{
                    context
                        .eval(*node.child("left"))
                        .asInt()
                };

                const int rhs{
                    context
                        .eval(*node.child("right"))
                        .asInt()
                };

                return runtime::Value::integer(
                    lhs + rhs);
            });

        language.runtime.expression(
            "program",
            [](const ast::Node &node,
               const runtime::RuntimeContext &context)
            {
                return context.eval(
                    *node.child("expression"));
            });

        //
        // HIR
        //

        language.lowering.hir(
            "integer",
            [](const ast::Node &node,
               ir::HIRBuilder &builder,
               const ir::LoweringRegistry &)
            {
                builder.emit(
                    "hir.const",
                    {
                        std::to_string(
                            node.integer("value"))
                    });
            });

        language.lowering.hir(
            "binary",
            [](const ast::Node &node,
               ir::HIRBuilder &builder,
               const ir::LoweringRegistry &registry)
            {
                registry.lowerHIR(
                    *node.child("left"),
                    builder);

                registry.lowerHIR(
                    *node.child("right"),
                    builder);

                builder.emit(
                    "hir.add");
            });

        language.lowering.hir(
            "program",
            [](const ast::Node &node,
               ir::HIRBuilder &builder,
               const ir::LoweringRegistry &registry)
            {
                registry.lowerHIR(
                    *node.child("expression"),
                    builder);
            });

        //
        // MIR
        //

        language.lowering.mir(
            "hir.const",
            [](const ir::HIRNode &node,
               ir::MIRBuilder &builder,
               const ir::LoweringRegistry &)
            {
                builder.emit(
                    "mir.const",
                    node.operands);
            });

        language.lowering.mir(
            "hir.add",
            [](const ir::HIRNode &,
               ir::MIRBuilder &builder,
               const ir::LoweringRegistry &)
            {
                builder.emit(
                    "mir.add");
            });

        //
        // BACKEND
        //

        class DumpBackend final
            : public backend::Backend
        {
        public:

            std::string name() const override
            {
                return "dump";
            }

            void emit(
                const ir::MIRModule &module) override
            {
                std::cout
                    << "\n=== MIR ===\n";

                for (const auto &node : module.nodes) {
                    std::cout
                        << node.op
                        << '\n';
                }
            }
        };

        language.backends.add(
            "dump",
            []()
            {
                return std::make_unique<DumpBackend>();
            });

        //
        // TEST
        //

        compiler::Compiler compiler{
            language,
            "program"
        };

        const std::string source{
            "40 + 2 + 8"
        };

        auto ast{
            compiler.parse(source)
        };

        auto hir{
            compiler.lowerToHIR(*ast)
        };

        auto mir{
            compiler.lowerToMIR(*ast)
        };

        compiler.emit(
            *ast,
            "dump");

        runtime::RuntimeContext context{
            language.runtime
        };

        auto result{
            context.eval(*ast)
        };

        std::cout
            << "\nResult = "
            << result.toString()
            << '\n';
    }
    catch (const std::exception &exception) {
        std::cerr
            << exception.what()
            << '\n';

        return 1;
    }

    return 0;
}