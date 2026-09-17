#include "../../tester.hpp"
#include "novac/engine/syntax/Parser.hpp"
#include "novac/engine/syntax/Node.hpp"
#include "novac/engine/syntax/Token.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace {

using novac::ast::Node;
using novac::ast::NodePtr;
using novac::parser::Associativity;
using novac::parser::Parser;
using novac::parser::ParserContext;
using novac::parser::ParserRegistry;
using novac::registry::RegisterStatus;
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

Token tok(Kind kind, std::string text, int line = 1, int column = 1) {
    return Token{kind, std::move(text), "", {}, line, column};
}

Token endTok(int line = 1, int column = 1) {
    return tok(Kind::End, "", line, column);
}

std::vector<Token> tokens(std::initializer_list<Token> values) {
    return std::vector<Token>{values};
}

NodePtr literal(int value) {
    NodePtr node{Node::make("IntegerLiteral")};
    node->set("value", value);
    return node;
}

NodePtr identifier(std::string name) {
    NodePtr node{Node::make("Identifier")};
    node->set("name", std::move(name));
    return node;
}

NodePtr prefixNode(std::string op, NodePtr right) {
    NodePtr node{Node::make("PrefixExpression")};
    node->set("op", std::move(op));
    node->set("right", std::move(right));
    return node;
}

NodePtr binary(NodePtr left, std::string op, NodePtr right) {
    NodePtr node{Node::make("BinaryExpression")};
    node->set("left", std::move(left));
    node->set("op", std::move(op));
    node->set("right", std::move(right));
    return node;
}

NodePtr postfixNode(NodePtr left, std::string op) {
    NodePtr node{Node::make("PostfixExpression")};
    node->set("left", std::move(left));
    node->set("op", std::move(op));
    return node;
}

void installIntegerPrefix(ParserRegistry &registry, const std::string &domain = "expr") {
    registry.prefix(domain, "$int", [](ParserContext &context) {
        const Token token{context.consumeKind(Kind::Integer)};
        return literal(std::stoi(token.text));
    });
}

void installIdentifierPrefix(ParserRegistry &registry, const std::string &domain = "expr") {
    registry.prefix(domain, "$identifier", [](ParserContext &context) {
        const Token token{context.consumeKind(Kind::Identifier)};
        return identifier(token.text);
    });
}

void installPrefixMinus(ParserRegistry &registry, const std::string &domain = "expr") {
    registry.prefix(domain, "-", [domain](ParserContext &context) {
        const Token op{context.consume("-")};
        return prefixNode(op.text, context.parse(domain, 100));
    });
}

void installBinary(ParserRegistry &registry, std::string op, int precedence, Associativity associativity = Associativity::Left, const std::string &domain = "expr") {
    registry.infix(domain, op, precedence, associativity, [](ParserContext &, NodePtr left, Token opToken, NodePtr right) {
        return binary(std::move(left), opToken.text, std::move(right));
    });
}

void installPostfix(ParserRegistry &registry, std::string op, int precedence, const std::string &domain = "expr") {
    registry.postfix(domain, op, precedence, [](ParserContext &, NodePtr left, Token opToken) {
        return postfixNode(std::move(left), opToken.text);
    });
}

ParserRegistry arithmeticRegistry() {
    ParserRegistry registry{};
    installIntegerPrefix(registry);
    installIdentifierPrefix(registry);
    installPrefixMinus(registry);
    installBinary(registry, "+", 10, Associativity::Left);
    installBinary(registry, "-", 10, Associativity::Left);
    installBinary(registry, "*", 20, Associativity::Left);
    installBinary(registry, "^", 30, Associativity::Right);
    installPostfix(registry, "!", 40);
    return registry;
}

} // namespace

TEST(ParserContext, CreatesEndTokenForEmptyTokenList) {
    ParserRegistry registry{};
    ParserContext context{{}, registry};

    CHECK(context.end());
    CHECK(context.cur().kind == Kind::End);
    CHECK(context.cur().line == 1);
    CHECK(context.cur().column == 1);
}

TEST(ParserContext, AppendsEndTokenWhenMissing) {
    ParserRegistry registry{};
    ParserContext context{tokens({tok(Kind::Identifier, "x", 2, 3)}), registry};

    CHECK(!context.end());
    CHECK(context.cur().text == "x");

    context.advance();

    CHECK(context.end());
    CHECK(context.cur().kind == Kind::End);
}

TEST(ParserContext, PreservesExistingEndToken) {
    ParserRegistry registry{};
    ParserContext context{tokens({tok(Kind::Identifier, "x", 2, 3), endTok(2, 4)}), registry};

    context.advance();

    CHECK(context.end());
    CHECK(context.cur().line == 2);
    CHECK(context.cur().column == 4);
}

TEST(ParserContext, CurReturnsCurrentToken) {
    ParserRegistry registry{};
    ParserContext context{tokens({tok(Kind::Identifier, "x", 2, 3), endTok(2, 4)}), registry};

    CHECK(context.cur().kind == Kind::Identifier);
    CHECK(context.cur().text == "x");
    CHECK(context.cur().line == 2);
    CHECK(context.cur().column == 3);
}

TEST(ParserContext, PeekReturnsLookaheadToken) {
    ParserRegistry registry{};
    ParserContext context{tokens({
        tok(Kind::Identifier, "a", 1, 1),
        tok(Kind::Symbol, "+", 1, 2),
        tok(Kind::Identifier, "b", 1, 3),
        endTok(1, 4)
    }), registry};

    CHECK(context.peek(0).text == "a");
    CHECK(context.peek(1).text == "+");
    CHECK(context.peek(2).text == "b");
}

TEST(ParserContext, PeekPastEndReturnsLastToken) {
    ParserRegistry registry{};
    ParserContext context{tokens({tok(Kind::Identifier, "a"), endTok()}), registry};

    CHECK(context.peek(100).kind == Kind::End);
}

TEST(ParserContext, CheckMatchesCurrentTokenText) {
    ParserRegistry registry{};
    ParserContext context{tokens({tok(Kind::Symbol, "+"), endTok()}), registry};

    CHECK(context.check("+"));
    CHECK(!context.check("-"));
}

TEST(ParserContext, AdvanceConsumesCurrentToken) {
    ParserRegistry registry{};
    ParserContext context{tokens({tok(Kind::Identifier, "a"), tok(Kind::Identifier, "b"), endTok()}), registry};

    const Token first{context.advance()};

    CHECK(first.text == "a");
    CHECK(context.cur().text == "b");

    const Token second{context.advance()};

    CHECK(second.text == "b");
    CHECK(context.end());
}

TEST(ParserContext, ConsumeMatchesText) {
    ParserRegistry registry{};
    ParserContext context{tokens({tok(Kind::Symbol, "+"), endTok()}), registry};

    const Token consumed{context.consume("+")};

    CHECK(consumed.text == "+");
    CHECK(context.end());
}

TEST(ParserContext, ConsumeRejectsUnexpectedText) {
    ParserRegistry registry{};
    ParserContext context{tokens({tok(Kind::Symbol, "+"), endTok()}), registry};

    CHECK(throwsRuntimeError([&]() { context.consume("-"); }));
}

TEST(ParserContext, ConsumeKindMatchesKind) {
    ParserRegistry registry{};
    ParserContext context{tokens({tok(Kind::Integer, "42"), endTok()}), registry};

    const Token consumed{context.consumeKind(Kind::Integer)};

    CHECK(consumed.kind == Kind::Integer);
    CHECK(consumed.text == "42");
    CHECK(context.end());
}

TEST(ParserContext, ConsumeKindRejectsUnexpectedKind) {
    ParserRegistry registry{};
    ParserContext context{tokens({tok(Kind::Integer, "42"), endTok()}), registry};

    CHECK(throwsRuntimeError([&]() { context.consumeKind(Kind::String); }));
}

TEST(ParserRegistry, RegistersParseRule) {
    ParserRegistry registry{};

    CHECK(registry.rule("expr", "$int", [](ParserContext &context) {
        context.consumeKind(Kind::Integer);
        return literal(1);
    }) == RegisterStatus::Inserted);
}

TEST(ParserRegistry, RejectsInvalidParseRules) {
    ParserRegistry registry{};

    CHECK(throwsRuntimeError([&]() { registry.rule("", "$int", [](ParserContext &) { return literal(1); }); }));
    CHECK(throwsRuntimeError([&]() { registry.rule("expr", "", [](ParserContext &) { return literal(1); }); }));
    CHECK(throwsRuntimeError([&]() { registry.rule("expr", "$int", {}); }));
}

TEST(ParserRegistry, ExecutesRuleMatchedByTokenKey) {
    ParserRegistry registry{};

    registry.rule("expr", "$int", [](ParserContext &context) {
        const Token token{context.consumeKind(Kind::Integer)};
        return literal(std::stoi(token.text));
    });

    ParserContext context{tokens({tok(Kind::Integer, "42"), endTok()}), registry};
    NodePtr node{registry.parse(context, "expr")};

    CHECK(node != nullptr);
    CHECK(node->kind() == "IntegerLiteral");
    CHECK(node->integer("value") == 42);
    CHECK(context.end());
}

TEST(ParserRegistry, ExecutesRuleMatchedByTokenText) {
    ParserRegistry registry{};

    registry.rule("stmt", "return", [](ParserContext &context) {
        context.consume("return");
        return Node::make("ReturnStatement");
    });

    ParserContext context{tokens({tok(Kind::Keyword, "return"), endTok()}), registry};
    NodePtr node{registry.parse(context, "stmt")};

    CHECK(node != nullptr);
    CHECK(node->kind() == "ReturnStatement");
    CHECK(context.end());
}

TEST(ParserRegistry, RejectsUnknownDomain) {
    ParserRegistry registry{};
    ParserContext context{tokens({endTok()}), registry};

    CHECK(throwsRuntimeError([&]() { registry.parse(context, "missing"); }));
}

TEST(ParserRegistry, RejectsWhenNoRuleMatches) {
    ParserRegistry registry{};

    registry.rule("expr", "$int", [](ParserContext &context) {
        context.consumeKind(Kind::Integer);
        return literal(1);
    });

    ParserContext context{tokens({tok(Kind::Identifier, "x"), endTok()}), registry};

    CHECK(throwsRuntimeError([&]() { registry.parse(context, "expr"); }));
}

TEST(ParserRegistry, RegistersAndExecutesFallbackRule) {
    ParserRegistry registry{};

    CHECK(registry.fallback("expr", [](ParserContext &context) -> NodePtr {
        if (context.cur().kind != Kind::Identifier) {
            return nullptr;
        }

        const Token token{context.consumeKind(Kind::Identifier)};
        return identifier(token.text);
    }) == RegisterStatus::Inserted);

    ParserContext context{tokens({tok(Kind::Identifier, "x"), endTok()}), registry};
    NodePtr node{registry.parse(context, "expr")};

    CHECK(node != nullptr);
    CHECK(node->kind() == "Identifier");
    CHECK(node->str("name") == "x");
    CHECK(context.end());
}

TEST(ParserRegistry, TriesFallbacksUntilOneReturnsNode) {
    ParserRegistry registry{};
    bool firstCalled{};
    bool secondCalled{};

    registry.fallback("expr", [&](ParserContext &) -> NodePtr {
        firstCalled = true;
        return nullptr;
    });

    registry.fallback("expr", [&](ParserContext &context) -> NodePtr {
        secondCalled = true;
        const Token token{context.consumeKind(Kind::Identifier)};
        return identifier(token.text);
    });

    ParserContext context{tokens({tok(Kind::Identifier, "x"), endTok()}), registry};
    NodePtr node{registry.parse(context, "expr")};

    CHECK(firstCalled);
    CHECK(secondCalled);
    CHECK(node != nullptr);
    CHECK(node->str("name") == "x");
}

TEST(ParserRegistry, RejectsInvalidFallbackRules) {
    ParserRegistry registry{};

    CHECK(throwsRuntimeError([&]() { registry.fallback("", [](ParserContext &) { return Node::make("Node"); }); }));
    CHECK(throwsRuntimeError([&]() { registry.fallback("expr", {}); }));
}

TEST(ParserRegistry, RegistersPrefixRule) {
    ParserRegistry registry{};

    CHECK(registry.prefix("expr", "$int", [](ParserContext &context) {
        const Token token{context.consumeKind(Kind::Integer)};
        return literal(std::stoi(token.text));
    }) == RegisterStatus::Inserted);
}

TEST(ParserRegistry, RejectsInvalidPrefixRules) {
    ParserRegistry registry{};

    CHECK(throwsRuntimeError([&]() { registry.prefix("", "$int", [](ParserContext &) { return literal(1); }); }));
    CHECK(throwsRuntimeError([&]() { registry.prefix("expr", "", [](ParserContext &) { return literal(1); }); }));
    CHECK(throwsRuntimeError([&]() { registry.prefix("expr", "$int", {}); }));
}

TEST(ParserRegistry, ParsesPrefixRuleByTokenKey) {
    ParserRegistry registry{};
    installIntegerPrefix(registry);

    ParserContext context{tokens({tok(Kind::Integer, "7"), endTok()}), registry};
    NodePtr node{registry.parse(context, "expr")};

    CHECK(node != nullptr);
    CHECK(node->kind() == "IntegerLiteral");
    CHECK(node->integer("value") == 7);
    CHECK(context.end());
}

TEST(ParserRegistry, ParsesPrefixRuleByTokenText) {
    ParserRegistry registry{};
    installIntegerPrefix(registry);
    installPrefixMinus(registry);

    ParserContext context{tokens({tok(Kind::Symbol, "-"), tok(Kind::Integer, "7"), endTok()}), registry};
    NodePtr node{registry.parse(context, "expr")};

    CHECK(node != nullptr);
    CHECK(node->kind() == "PrefixExpression");
    CHECK(node->str("op") == "-");
    CHECK(node->child("right")->integer("value") == 7);
    CHECK(context.end());
}

TEST(ParserRegistry, RegistersInfixRule) {
    ParserRegistry registry{};

    CHECK(registry.infix("expr", "+", 10, [](ParserContext &, NodePtr left, Token op, NodePtr right) {
        return binary(std::move(left), op.text, std::move(right));
    }) == RegisterStatus::Inserted);
}

TEST(ParserRegistry, RegistersInfixRuleWithAssociativity) {
    ParserRegistry registry{};

    CHECK(registry.infix("expr", "^", 20, Associativity::Right, [](ParserContext &, NodePtr left, Token op, NodePtr right) {
        return binary(std::move(left), op.text, std::move(right));
    }) == RegisterStatus::Inserted);
}

TEST(ParserRegistry, RejectsInvalidInfixRules) {
    ParserRegistry registry{};

    CHECK(throwsRuntimeError([&]() {
        registry.infix("", "+", 10, [](ParserContext &, NodePtr left, Token op, NodePtr right) {
            return binary(std::move(left), op.text, std::move(right));
        });
    }));

    CHECK(throwsRuntimeError([&]() {
        registry.infix("expr", "", 10, [](ParserContext &, NodePtr left, Token op, NodePtr right) {
            return binary(std::move(left), op.text, std::move(right));
        });
    }));

    CHECK(throwsRuntimeError([&]() {
        registry.infix("expr", "+", 10, {});
    }));
}

TEST(ParserRegistry, ParsesInfixExpression) {
    ParserRegistry registry{arithmeticRegistry()};

    ParserContext context{tokens({
        tok(Kind::Integer, "1"),
        tok(Kind::Symbol, "+"),
        tok(Kind::Integer, "2"),
        endTok()
    }), registry};

    NodePtr node{registry.parse(context, "expr")};

    CHECK(node != nullptr);
    CHECK(node->kind() == "BinaryExpression");
    CHECK(node->str("op") == "+");
    CHECK(node->child("left")->integer("value") == 1);
    CHECK(node->child("right")->integer("value") == 2);
    CHECK(context.end());
}

TEST(ParserRegistry, RespectsOperatorPrecedence) {
    ParserRegistry registry{arithmeticRegistry()};

    ParserContext context{tokens({
        tok(Kind::Integer, "1"),
        tok(Kind::Symbol, "+"),
        tok(Kind::Integer, "2"),
        tok(Kind::Symbol, "*"),
        tok(Kind::Integer, "3"),
        endTok()
    }), registry};

    NodePtr root{registry.parse(context, "expr")};

    CHECK(root->str("op") == "+");
    CHECK(root->child("left")->integer("value") == 1);

    NodePtr right{root->child("right")};
    CHECK(right->kind() == "BinaryExpression");
    CHECK(right->str("op") == "*");
    CHECK(right->child("left")->integer("value") == 2);
    CHECK(right->child("right")->integer("value") == 3);
}

TEST(ParserRegistry, RespectsLeftAssociativity) {
    ParserRegistry registry{arithmeticRegistry()};

    ParserContext context{tokens({
        tok(Kind::Integer, "1"),
        tok(Kind::Symbol, "-"),
        tok(Kind::Integer, "2"),
        tok(Kind::Symbol, "-"),
        tok(Kind::Integer, "3"),
        endTok()
    }), registry};

    NodePtr root{registry.parse(context, "expr")};

    CHECK(root->str("op") == "-");
    CHECK(root->child("right")->integer("value") == 3);

    NodePtr left{root->child("left")};
    CHECK(left->kind() == "BinaryExpression");
    CHECK(left->str("op") == "-");
    CHECK(left->child("left")->integer("value") == 1);
    CHECK(left->child("right")->integer("value") == 2);
}

TEST(ParserRegistry, RespectsRightAssociativity) {
    ParserRegistry registry{arithmeticRegistry()};

    ParserContext context{tokens({
        tok(Kind::Integer, "2"),
        tok(Kind::Symbol, "^"),
        tok(Kind::Integer, "3"),
        tok(Kind::Symbol, "^"),
        tok(Kind::Integer, "4"),
        endTok()
    }), registry};

    NodePtr root{registry.parse(context, "expr")};

    CHECK(root->str("op") == "^");
    CHECK(root->child("left")->integer("value") == 2);

    NodePtr right{root->child("right")};
    CHECK(right->kind() == "BinaryExpression");
    CHECK(right->str("op") == "^");
    CHECK(right->child("left")->integer("value") == 3);
    CHECK(right->child("right")->integer("value") == 4);
}

TEST(ParserRegistry, RegistersPostfixRule) {
    ParserRegistry registry{};

    CHECK(registry.postfix("expr", "!", 40, [](ParserContext &, NodePtr left, Token op) {
        return postfixNode(std::move(left), op.text);
    }) == RegisterStatus::Inserted);
}

TEST(ParserRegistry, RejectsInvalidPostfixRules) {
    ParserRegistry registry{};

    CHECK(throwsRuntimeError([&]() {
        registry.postfix("", "!", 40, [](ParserContext &, NodePtr left, Token op) {
            return postfixNode(std::move(left), op.text);
        });
    }));

    CHECK(throwsRuntimeError([&]() {
        registry.postfix("expr", "", 40, [](ParserContext &, NodePtr left, Token op) {
            return postfixNode(std::move(left), op.text);
        });
    }));

    CHECK(throwsRuntimeError([&]() { registry.postfix("expr", "!", 40, {}); }));
}

TEST(ParserRegistry, ParsesPostfixExpression) {
    ParserRegistry registry{arithmeticRegistry()};

    ParserContext context{tokens({
        tok(Kind::Integer, "5"),
        tok(Kind::Symbol, "!"),
        endTok()
    }), registry};

    NodePtr node{registry.parse(context, "expr")};

    CHECK(node != nullptr);
    CHECK(node->kind() == "PostfixExpression");
    CHECK(node->str("op") == "!");
    CHECK(node->child("left")->integer("value") == 5);
}

TEST(ParserRegistry, PostfixBindsTighterThanInfix) {
    ParserRegistry registry{arithmeticRegistry()};

    ParserContext context{tokens({
        tok(Kind::Integer, "1"),
        tok(Kind::Symbol, "+"),
        tok(Kind::Integer, "2"),
        tok(Kind::Symbol, "!"),
        endTok()
    }), registry};

    NodePtr root{registry.parse(context, "expr")};

    CHECK(root->kind() == "BinaryExpression");
    CHECK(root->str("op") == "+");
    CHECK(root->child("left")->integer("value") == 1);

    NodePtr right{root->child("right")};
    CHECK(right->kind() == "PostfixExpression");
    CHECK(right->str("op") == "!");
    CHECK(right->child("left")->integer("value") == 2);
}

TEST(ParserContext, ParseDelegatesToRegistry) {
    ParserRegistry registry{};
    installIntegerPrefix(registry);

    ParserContext context{tokens({tok(Kind::Integer, "123"), endTok()}), registry};
    NodePtr node{context.parse("expr")};

    CHECK(node != nullptr);
    CHECK(node->integer("value") == 123);
    CHECK(context.end());
}

TEST(ParserContext, ParseDelegatesToRegistryWithTypedDomain) {
    ParserRegistry registry{};
    installIntegerPrefix(registry, "typed");

    ParserContext context{tokens({tok(Kind::Integer, "321"), endTok()}), registry};
    novac::ids::ParseDomain domain{"typed"};
    NodePtr node{context.parse(domain)};

    CHECK(node != nullptr);
    CHECK(node->integer("value") == 321);
    CHECK(context.end());
}

TEST(Parser, RejectsEmptyStartDomain) {
    ParserRegistry registry{};

    CHECK(throwsRuntimeError([&]() { Parser parser{registry, ""}; }));
}

TEST(Parser, ParsesUsingStartDomain) {
    ParserRegistry registry{arithmeticRegistry()};
    Parser parser{registry, "expr"};

    NodePtr node{parser.parse(tokens({
        tok(Kind::Integer, "1"),
        tok(Kind::Symbol, "+"),
        tok(Kind::Integer, "2"),
        endTok()
    }))};

    CHECK(node != nullptr);
    CHECK(node->kind() == "BinaryExpression");
    CHECK(node->str("op") == "+");
}

TEST(Parser, ParsesUsingTypedStartDomain) {
    ParserRegistry registry{};
    installIntegerPrefix(registry, "typed");

    novac::ids::ParseDomain domain{"typed"};
    Parser parser{registry, domain};

    NodePtr node{parser.parse(tokens({tok(Kind::Integer, "9"), endTok()}))};

    CHECK(node != nullptr);
    CHECK(node->kind() == "IntegerLiteral");
    CHECK(node->integer("value") == 9);
}

TEST(Parser, RejectsTrailingTokensAfterCompleteParse) {
    ParserRegistry registry{};
    installIntegerPrefix(registry);
    Parser parser{registry, "expr"};

    CHECK(throwsRuntimeError([&]() {
        parser.parse(tokens({
            tok(Kind::Integer, "1"),
            tok(Kind::Integer, "2"),
            endTok()
        }));
    }));
}

TEST(Parser, ParsePartialAllowsTrailingTokens) {
    ParserRegistry registry{};
    installIntegerPrefix(registry);
    Parser parser{registry, "expr"};

    NodePtr node{parser.parsePartial(tokens({
        tok(Kind::Integer, "1"),
        tok(Kind::Integer, "2"),
        endTok()
    }))};

    CHECK(node != nullptr);
    CHECK(node->kind() == "IntegerLiteral");
    CHECK(node->integer("value") == 1);
}

TEST(Parser, RejectsUnknownStartDomainOnParse) {
    ParserRegistry registry{};
    Parser parser{registry, "missing"};

    CHECK(throwsRuntimeError([&]() { parser.parse(tokens({endTok()})); }));
}
