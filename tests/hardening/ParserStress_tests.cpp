#include "../tester.hpp"

#include "novac/engine/syntax/Node.hpp"
#include "novac/engine/syntax/Parser.hpp"
#include "novac/engine/syntax/Token.hpp"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using novac::ast::Node;
using novac::ast::NodePtr;
using novac::parser::Associativity;
using novac::parser::ParserContext;
using novac::parser::ParserRegistry;
using novac::token::Kind;
using novac::token::Token;

template <typename Fn>
bool throwsRuntimeError(Fn &&fn) {
    try {
        fn();
    } catch (const std::runtime_error &) {
        return true;
    } catch (...) {
        return false;
    }

    return false;
}

Token tok(Kind kind, std::string text) {
    return Token{kind, std::move(text), "", {}, 1, 1};
}

Token endTok() {
    return tok(Kind::End, "");
}

NodePtr integerNode(int value) {
    NodePtr node{Node::make("IntegerLiteral")};
    node->set("value", value);
    return node;
}

NodePtr identifierNode(std::string name) {
    NodePtr node{Node::make("Identifier")};
    node->set("name", std::move(name));
    return node;
}

NodePtr binaryNode(NodePtr left, std::string op, NodePtr right) {
    NodePtr node{Node::make("BinaryExpression")};
    node->set("left", std::move(left));
    node->set("op", std::move(op));
    node->set("right", std::move(right));
    return node;
}

void installIntegerPrefix(ParserRegistry &registry) {
    registry.prefix("expr", "$int", [](ParserContext &context) {
        const Token token{context.consumeKind(Kind::Integer)};
        return integerNode(std::stoi(token.text));
    });
}

void installIdentifierPrefix(ParserRegistry &registry) {
    registry.prefix("expr", "$identifier", [](ParserContext &context) {
        const Token token{context.consumeKind(Kind::Identifier)};
        return identifierNode(token.text);
    });
}

void installPlus(ParserRegistry &registry) {
    registry.infix(
        "expr",
        "+",
        10,
        Associativity::Left,
        [](ParserContext &, NodePtr left, Token op, NodePtr right) {
            return binaryNode(std::move(left), op.text, std::move(right));
        });
}

TEST(ParserStress, EmptyExpressionFailsCleanly) {
    ParserRegistry registry{};
    installIntegerPrefix(registry);

    ParserContext context{{endTok()}, registry};

    CHECK(throwsRuntimeError([&]() {
        static_cast<void>(registry.parse(context, "expr"));
    }));
    CHECK(context.end());
}

TEST(ParserStress, InfixWithoutRightOperandFailsAtEnd) {
    ParserRegistry registry{};
    installIntegerPrefix(registry);
    installPlus(registry);

    ParserContext context{{
        tok(Kind::Integer, "1"),
        tok(Kind::Symbol, "+"),
        endTok()
    }, registry};

    CHECK(throwsRuntimeError([&]() {
        static_cast<void>(registry.parse(context, "expr"));
    }));
    CHECK(context.end());
}

TEST(ParserStress, UnexpectedOperatorWhereOperandIsRequiredFailsCleanly) {
    ParserRegistry registry{};
    installIntegerPrefix(registry);
    installPlus(registry);

    ParserContext context{{
        tok(Kind::Integer, "1"),
        tok(Kind::Symbol, "+"),
        tok(Kind::Symbol, "+"),
        tok(Kind::Integer, "2"),
        endTok()
    }, registry};

    CHECK(throwsRuntimeError([&]() {
        static_cast<void>(registry.parse(context, "expr"));
    }));
}

TEST(ParserStress, FailedFallbacksRestoreAcrossMultipleConsumedTokens) {
    ParserRegistry registry{};

    registry.fallback("expr", [](ParserContext &context) -> NodePtr {
        context.consumeKind(Kind::Identifier);
        context.consume("(");
        return nullptr;
    });

    registry.fallback("expr", [](ParserContext &context) -> NodePtr {
        const Token name{context.consumeKind(Kind::Identifier)};
        context.consume("(");
        context.consume(")");
        return identifierNode(name.text);
    });

    ParserContext context{{
        tok(Kind::Identifier, "call"),
        tok(Kind::Symbol, "("),
        tok(Kind::Symbol, ")"),
        endTok()
    }, registry};

    const NodePtr node{registry.parse(context, "expr")};
    CHECK(node->kind() == "Identifier");
    CHECK(node->str("name") == "call");
    CHECK(context.end());
}

TEST(ParserStress, PrefixFallbacksRollbackBetweenCandidates) {
    ParserRegistry registry{};

    registry.prefixFallback("expr", "$identifier", [](ParserContext &context) -> NodePtr {
        context.consumeKind(Kind::Identifier);
        context.consume("(");
        return nullptr;
    });

    registry.prefixFallback("expr", "$identifier", [](ParserContext &context) -> NodePtr {
        const Token name{context.consumeKind(Kind::Identifier)};
        context.consume("(");
        context.consume(")");
        return identifierNode(name.text);
    });

    ParserContext context{{
        tok(Kind::Identifier, "call"),
        tok(Kind::Symbol, "("),
        tok(Kind::Symbol, ")"),
        endTok()
    }, registry};

    const NodePtr node{registry.parse(context, "expr")};
    CHECK(node->str("name") == "call");
    CHECK(context.end());
}

TEST(ParserStress, RejectedPrefixFallbackFallsThroughToPrimaryPrefix) {
    ParserRegistry registry{};

    registry.prefixFallback("expr", "$identifier", [](ParserContext &context) -> NodePtr {
        context.consumeKind(Kind::Identifier);
        return nullptr;
    });
    installIdentifierPrefix(registry);

    ParserContext context{{tok(Kind::Identifier, "value"), endTok()}, registry};
    const NodePtr node{registry.parse(context, "expr")};

    CHECK(node->str("name") == "value");
    CHECK(context.end());
}

TEST(ParserStress, RejectedPrefixFallbackFallsThroughToClassicFallback) {
    ParserRegistry registry{};

    registry.prefixFallback("expr", "$identifier", [](ParserContext &context) -> NodePtr {
        context.consumeKind(Kind::Identifier);
        return nullptr;
    });

    registry.fallback("expr", [](ParserContext &context) -> NodePtr {
        const Token name{context.consumeKind(Kind::Identifier)};
        return identifierNode(name.text);
    });

    ParserContext context{{tok(Kind::Identifier, "value"), endTok()}, registry};
    const NodePtr node{registry.parse(context, "expr")};

    CHECK(node->str("name") == "value");
    CHECK(context.end());
}

TEST(ParserStress, PrefixFallbackResultStillParticipatesInInfixParsing) {
    ParserRegistry registry{};
    installIntegerPrefix(registry);
    installPlus(registry);

    registry.prefixFallback("expr", "$identifier", [](ParserContext &context) -> NodePtr {
        if (context.peek().text != "(")
            return nullptr;

        const Token name{context.consumeKind(Kind::Identifier)};
        context.consume("(");
        context.consume(")");
        return identifierNode(name.text);
    });

    ParserContext context{{
        tok(Kind::Identifier, "call"),
        tok(Kind::Symbol, "("),
        tok(Kind::Symbol, ")"),
        tok(Kind::Symbol, "+"),
        tok(Kind::Integer, "1"),
        endTok()
    }, registry};

    const NodePtr node{registry.parse(context, "expr")};
    CHECK(node->kind() == "BinaryExpression");
    CHECK(node->child("left")->str("name") == "call");
    CHECK(node->child("right")->integer("value") == 1);
    CHECK(context.end());
}

} // namespace
