#include "include/novac/assets/compilation/compiler/Compiler.hpp"
#include "include/novac/assets/compilation/compiler/Pass.hpp"
#include "include/novac/assets/language/Language.hpp"
#include "include/novac/assets/semantic/Semantic.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

using namespace novac;

namespace testlang::nodes {

inline const ids::NodeKind program{"program"};
inline const ids::NodeKind integer{"integer"};
inline const ids::NodeKind binary{"binary"};

} // namespace testlang::nodes

namespace testlang::fields {

inline const ids::FieldName expression{"expression"};
inline const ids::FieldName value{"value"};
inline const ids::FieldName left{"left"};
inline const ids::FieldName right{"right"};
inline const ids::FieldName op{"op"};

} // namespace testlang::fields

namespace testlang::domains {

inline const ids::ParseDomain program{"program"};
inline const ids::ParseDomain expr{"expr"};

} // namespace testlang::domains

namespace testlang::ops {

inline const ids::Operation add{"+"};
inline const ids::Operation hirConst{"hir.const"};
inline const ids::Operation hirAdd{"hir.add"};
inline const ids::Operation mirConst{"mir.const"};
inline const ids::Operation mirAdd{"mir.add"};

} // namespace testlang::ops

class DumpSemanticPass final : public compiler::Pass {
public:
    std::string name() const override
    {
        return "dump.semantic";
    }

    void run(compiler::CompilationContext &context) const override
    {
        const semantic::SemanticContext &semanticContext{
            context.requireArtifact<semantic::SemanticContext>("semantic")
        };

        std::cout << "\n=== SEMANTIC ===\n";
        std::cout << "Scopes: " << semanticContext.scopes().size() << '\n';
        std::cout << "Symbols: " << semanticContext.symbols().size() << '\n';
        std::cout << "References: " << semanticContext.references().size() << '\n';
    }
};

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

        for (const ir::BasicBlock &block : module.blocks) {
            std::cout << block.name << ":\n";

            for (const ir::Instruction &instruction : block.instructions) {
                std::cout << "  ";

                if (instruction.result) {
                    std::cout << "v" << instruction.result->value << " = ";
                }

                std::cout << instruction.op;

                for (const ir::Operand &operand : instruction.operands) {
                    std::cout << " " << operand.toString();
                }

                std::cout << " : " << ir::valueTypeName(instruction.type);
                std::cout << '\n';
            }
        }
    }
};

static ir::ValueId requireValue(
    const ir::LoweringRegistry::LoweredValue &value,
    const std::string &owner)
{
    if (!value) {
        throw std::runtime_error(
            owner + ": expected value-producing expression");
    }

    return *value;
}

int main()
{
    try {
        language::Language language{};

        language.lexer.symbol(testlang::ops::add.value);

        language.nodes.registerNode({
            testlang::nodes::program.value,
            {
                {testlang::fields::expression.value, ast::FieldKind::Node}
            },
            "program root"
        });

        language.nodes.registerNode({
            testlang::nodes::integer.value,
            {
                {testlang::fields::value.value, ast::FieldKind::Int}
            },
            "integer literal"
        });

        language.nodes.registerNode({
            testlang::nodes::binary.value,
            {
                {testlang::fields::left.value, ast::FieldKind::Node},
                {testlang::fields::right.value, ast::FieldKind::Node},
                {testlang::fields::op.value, ast::FieldKind::String}
            },
            "binary operation"
        });

        language.parser.prefix(
            testlang::domains::expr,
            "$int",
            [](parser::ParserContext &context) {
                const auto token{
                    context.consumeKind(token::Kind::Integer)
                };

                ast::NodePtr node{
                    ast::Node::make(testlang::nodes::integer)
                };

                node->set(
                    testlang::fields::value,
                    std::stoi(token.text));

                return node;
            });

        language.parser.infix(
            testlang::domains::expr,
            testlang::ops::add.value,
            10,
            [](parser::ParserContext &context, ast::NodePtr left) {
                context.consume(testlang::ops::add.value);

                ast::NodePtr right{
                    context.parse(testlang::domains::expr, 11)
                };

                ast::NodePtr node{
                    ast::Node::make(testlang::nodes::binary)
                };

                node->set(testlang::fields::left, left);
                node->set(testlang::fields::right, right);
                node->set(testlang::fields::op, testlang::ops::add.value);

                return node;
            });

        language.parser.fallback(
            testlang::domains::program,
            [](parser::ParserContext &context) {
                ast::NodePtr expression{
                    context.parse(testlang::domains::expr)
                };

                ast::NodePtr program{
                    ast::Node::make(testlang::nodes::program)
                };

                program->set(
                    testlang::fields::expression,
                    expression);

                return program;
            });

        language.runtime.expression(
            testlang::nodes::integer,
            [](const ast::Node &node, const runtime::RuntimeContext &) {
                return runtime::Value::integer(
                    node.integer(testlang::fields::value));
            });

        language.runtime.binaryOperator(
            testlang::ops::add,
            [](const ast::Node &node, const runtime::RuntimeContext &context) {
                const int lhs{
                    context.eval(*node.child(testlang::fields::left)).asInt()
                };

                const int rhs{
                    context.eval(*node.child(testlang::fields::right)).asInt()
                };

                return runtime::Value::integer(lhs + rhs);
            });

        language.runtime.expression(
            testlang::nodes::program,
            [](const ast::Node &node, const runtime::RuntimeContext &context) {
                return context.eval(
                    *node.child(testlang::fields::expression));
            });

        language.lowering.hir(
            testlang::nodes::integer,
            [](const ast::Node &node,
               ir::HIRBuilder &builder,
               const ir::LoweringRegistry &) -> ir::LoweringRegistry::LoweredValue {
                return builder.emitValue(
                    testlang::ops::hirConst,
                    {
                        ir::Operand::fromLiteral(
                            node.integer(testlang::fields::value))
                    },
                    ir::ValueType::Int);
            });

        language.lowering.hir(
            testlang::nodes::binary,
            [](const ast::Node &node,
               ir::HIRBuilder &builder,
               const ir::LoweringRegistry &registry) -> ir::LoweringRegistry::LoweredValue {
                const ir::ValueId lhs{
                    requireValue(
                        registry.lowerHIR(*node.child(testlang::fields::left), builder),
                        "binary.hir.left")
                };

                const ir::ValueId rhs{
                    requireValue(
                        registry.lowerHIR(*node.child(testlang::fields::right), builder),
                        "binary.hir.right")
                };

                return builder.emitValue(
                    testlang::ops::hirAdd,
                    {
                        ir::Operand::fromValue(lhs),
                        ir::Operand::fromValue(rhs)
                    },
                    ir::ValueType::Int);
            });

        language.lowering.hir(
            testlang::nodes::program,
            [](const ast::Node &node,
               ir::HIRBuilder &builder,
               const ir::LoweringRegistry &registry) -> ir::LoweringRegistry::LoweredValue {
                return registry.lowerHIR(
                    *node.child(testlang::fields::expression),
                    builder);
            });

        language.lowering.mir(
            testlang::ops::hirConst,
            [](const ir::Instruction &instruction,
               ir::MIRBuilder &builder,
               const ir::LoweringRegistry &) -> ir::LoweringRegistry::LoweredValue {
                return builder.emitValue(
                    testlang::ops::mirConst,
                    instruction.operands,
                    instruction.type);
            });

        language.lowering.mir(
            testlang::ops::hirAdd,
            [](const ir::Instruction &instruction,
               ir::MIRBuilder &builder,
               const ir::LoweringRegistry &) -> ir::LoweringRegistry::LoweredValue {
                return builder.emitValue(
                    testlang::ops::mirAdd,
                    instruction.operands,
                    instruction.type);
            });

        compiler::Compiler compiler{
            language,
            testlang::domains::program.value
        };

        compiler.addPass<compiler::ParsePass>("ast");
        compiler.addPass<compiler::AstValidationPass>("ast");
        compiler.addPass<compiler::SemanticPass>("ast", "semantic");
        compiler.addPass<DumpSemanticPass>();
        compiler.addPass<compiler::HIRLoweringPass>("ast", "hir");
        compiler.addPass<compiler::MIRLoweringPass>("hir", "mir");
        compiler.addPass<DumpMIRPass>();
        compiler.addPass<compiler::RuntimePass>("ast", "result");

        const std::string source{
            "40 + 2 + 8"
        };

        compiler::CompilationContext context{
            compiler.run(source)
        };

        const runtime::Value &result{
            context.requireArtifact<runtime::Value>("result")
        };

        std::cout
            << "\nResult = "
            << result.toString()
            << '\n';

        return 0;
    } catch (const std::exception &exception) {
        std::cerr
            << exception.what()
            << '\n';

        return 1;
    }
}