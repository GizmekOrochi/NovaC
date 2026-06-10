#include <iostream>
#include <memory>

#include "include/novac/engine/EngineController.hpp"

using namespace novac;

int main()
{
    controllers::EngineController engine{};

    engine.setStartDomain("expr");

    //
    // Lexer
    //
    engine.symbol("+");
    engine.symbol("-");
    engine.symbol("*");
    engine.symbol("/");
    engine.symbol("(");
    engine.symbol(")");

    //
    // AST
    //
    engine.node({
        .kind = "IntegerLiteral",
        .fields = {
            {
                .name = "value",
                .kind = ast::FieldKind::Int,
                .required = true
            }
        }
    });

    engine.node({
        .kind = "BinaryExpr",
        .fields = {
            {
                .name = "op",
                .kind = ast::FieldKind::String,
                .required = true
            },
            {
                .name = "left",
                .kind = ast::FieldKind::Node,
                .required = true
            },
            {
                .name = "right",
                .kind = ast::FieldKind::Node,
                .required = true
            }
        }
    });

    //
    // Parser : integer literal
    //
    engine.prefix(
        "expr",
        "$int",
        [&](parser::ParserContext& ctx)
        {
            const auto token =
                ctx.consumeKind(token::Kind::Integer);

            auto node =
                engine.makeNode("IntegerLiteral");

            node->set(
                "value",
                std::stoi(token.text));

            return node;
        });

    //
    // Parser : parenthesis
    //
    engine.prefix(
        "expr",
        "(",
        [&](parser::ParserContext& ctx)
        {
            ctx.consume("(");

            auto expr = ctx.parse("expr");

            ctx.consume(")");

            return expr;
        });

    //
    // Pratt helpers
    //
    auto binaryBuilder =
        [&](parser::ParserContext&,
            ast::NodePtr left,
            const token::Token& op,
            ast::NodePtr right)
        {
            auto node =
                engine.makeNode("BinaryExpr");

            node->set("op", op.text);
            node->set("left", left);
            node->set("right", right);

            return node;
        };

    engine.infix("expr", "+", 10, binaryBuilder);
    engine.infix("expr", "-", 10, binaryBuilder);

    engine.infix("expr", "*", 20, binaryBuilder);
    engine.infix("expr", "/", 20, binaryBuilder);

    //
    // Runtime
    //
    engine.expression(
        "IntegerLiteral",
        [](const ast::Node& node,
           runtime::RuntimeContext&)
        {
            return runtime::Value::integer(
                node.integer("value"));
        });

    //
    // IMPORTANT
    //
    engine.setBinaryNodeKind("BinaryExpr");

    engine.binaryOperator(
        "+",
        [](const ast::Node& node,
           runtime::RuntimeContext& ctx)
        {
            const int lhs = ctx.eval(*node.child("left")).asInt();
            const int rhs = ctx.eval(*node.child("right")).asInt();

            return runtime::Value::integer(
                lhs + rhs);
        });

    engine.binaryOperator(
        "-",
        [](const ast::Node& node,
           runtime::RuntimeContext& ctx)
        {
            const int lhs = ctx.eval(*node.child("left")).asInt();
            const int rhs = ctx.eval(*node.child("right")).asInt();

            return runtime::Value::integer(lhs - rhs);
        });

    engine.binaryOperator(
        "*",
        [](const ast::Node& node,
           runtime::RuntimeContext& ctx)
        {
            const int lhs =
                ctx.eval(*node.child("left"))
                    .asInt();

            const int rhs =
                ctx.eval(*node.child("right"))
                    .asInt();

            return runtime::Value::integer(
                lhs * rhs);
        });

    engine.binaryOperator(
        "/",
        [](const ast::Node& node,
           runtime::RuntimeContext& ctx)
        {
            const int lhs =
                ctx.eval(*node.child("left"))
                    .asInt();

            const int rhs =
                ctx.eval(*node.child("right"))
                    .asInt();

            return runtime::Value::integer(
                lhs / rhs);
        });

    //
    // Test
    //
    const std::string source =
        "10 + 20 * (3 + 2)";

    auto ast = engine.parse(source);

    engine.validate(*ast);

    auto result = engine.eval(*ast);

    std::cout
        << source
        << " = "
        << result.asInt()
        << '\n';

    return 0;
}