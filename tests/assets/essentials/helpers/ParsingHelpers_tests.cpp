#include "../../../tester.hpp"

#include "novac/assets/essentials/helpers/ParsingHelpers.hpp"
#include "novac/engine/syntax/Node.hpp"
#include "novac/engine/syntax/Parser.hpp"
#include "novac/engine/syntax/Token.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace {

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

using novac::ast::Node;
using novac::ast::NodePtr;
using novac::parser::ParserContext;
using novac::parser::ParserRegistry;
using novac::token::Kind;
using novac::token::Token;

Token tok(Kind kind, std::string text) {
    return Token{kind, std::move(text), "", {}, 1, 1};
}

Token endTok() {
    return tok(Kind::End, "");
}

std::vector<Token> tokens(std::initializer_list<Token> values) {
    return std::vector<Token>{values};
}

void installIntegerPrefix(ParserRegistry &registry) {
    registry.prefix("expr", "$int", [](ParserContext &context) {
        const Token token{context.consumeKind(Kind::Integer)};
        NodePtr node{Node::make("IntegerLiteral")};
        node->set("value", std::stoi(token.text));
        return node;
    });
}

TEST(ParsingHelpers, ConsumeIdentifierReturnsText) {
    ParserRegistry registry{};
    ParserContext context{tokens({tok(Kind::Identifier, "answer"), endTok()}), registry};

    CHECK(novac::assets::essentials::helpers::consumeIdentifier(context, "test") == "answer");
    CHECK(context.end());
}

TEST(ParsingHelpers, ConsumeIdentifierRejectsWrongTokenKind) {
    ParserRegistry registry{};
    ParserContext context{tokens({tok(Kind::Integer, "42"), endTok()}), registry};

    CHECK(throwsRuntimeError([&]() {(void)novac::assets::essentials::helpers::consumeIdentifier(context, "test");}));
}

TEST(ParsingHelpers, ConsumeIdentifierRejectsEmptyIdentifier) {
    ParserRegistry registry{};
    ParserContext context{tokens({tok(Kind::Identifier, ""), endTok()}), registry};

    CHECK(throwsRuntimeError([&]() {(void)novac::assets::essentials::helpers::consumeIdentifier(context, "test");}));
}

TEST(ParsingHelpers, ParseIdentifierListEmpty) {
    ParserRegistry registry{};
    ParserContext context{tokens({tok(Kind::Symbol, "("), tok(Kind::Symbol, ")"), endTok()}), registry};

    const auto names{novac::assets::essentials::helpers::parseIdentifierList(context, "(", ")", ",")};

    CHECK(names.empty());
    CHECK(context.end());
}

TEST(ParsingHelpers, ParseIdentifierListSingle) {
    ParserRegistry registry{};
    ParserContext context{tokens({tok(Kind::Symbol, "("), tok(Kind::Identifier, "value"), tok(Kind::Symbol, ")"), endTok()}), registry};

    const auto names{novac::assets::essentials::helpers::parseIdentifierList(context, "(", ")", ",")};

    CHECK(names.size() == 1);
    CHECK(names[0] == "value");
    CHECK(context.end());
}

TEST(ParsingHelpers, ParseIdentifierListMultiple) {
    ParserRegistry registry{};
    ParserContext context{tokens({
        tok(Kind::Symbol, "("),
        tok(Kind::Identifier, "left"),
        tok(Kind::Symbol, ","),
        tok(Kind::Identifier, "right"),
        tok(Kind::Symbol, ")"),
        endTok()
    }), registry};

    const auto names{novac::assets::essentials::helpers::parseIdentifierList(context, "(", ")", ",")};

    CHECK(names.size() == 2);
    CHECK(names[0] == "left");
    CHECK(names[1] == "right");
    CHECK(context.end());
}

TEST(ParsingHelpers, ParseIdentifierListRejectsTrailingComma) {
    ParserRegistry registry{};
    ParserContext context{tokens({
        tok(Kind::Symbol, "("),
        tok(Kind::Identifier, "value"),
        tok(Kind::Symbol, ","),
        tok(Kind::Symbol, ")"),
        endTok()
    }), registry};

    CHECK(throwsRuntimeError([&]() {(void)novac::assets::essentials::helpers::parseIdentifierList(context, "(", ")", ",");}));
}

TEST(ParsingHelpers, ParseExpressionListEmpty) {
    ParserRegistry registry{};
    installIntegerPrefix(registry);
    ParserContext context{tokens({tok(Kind::Symbol, "("), tok(Kind::Symbol, ")"), endTok()}), registry};

    const auto arguments{novac::assets::essentials::helpers::parseExpressionList(context, "expr", "(", ")", ",")};

    CHECK(arguments.empty());
    CHECK(context.end());
}

TEST(ParsingHelpers, ParseExpressionListSingle) {
    ParserRegistry registry{};
    installIntegerPrefix(registry);
    ParserContext context{tokens({
        tok(Kind::Symbol, "("),
        tok(Kind::Integer, "42"),
        tok(Kind::Symbol, ")"),
        endTok()
    }), registry};

    const auto arguments{novac::assets::essentials::helpers::parseExpressionList(context, "expr", "(", ")", ",")};

    CHECK(arguments.size() == 1);
    CHECK(arguments[0]->integer("value") == 42);
    CHECK(context.end());
}

TEST(ParsingHelpers, ParseExpressionListMultiple) {
    ParserRegistry registry{};
    installIntegerPrefix(registry);
    ParserContext context{tokens({
        tok(Kind::Symbol, "("),
        tok(Kind::Integer, "20"),
        tok(Kind::Symbol, ","),
        tok(Kind::Integer, "22"),
        tok(Kind::Symbol, ")"),
        endTok()
    }), registry};

    const auto arguments{novac::assets::essentials::helpers::parseExpressionList(context, "expr", "(", ")", ",")};

    CHECK(arguments.size() == 2);
    CHECK(arguments[0]->integer("value") == 20);
    CHECK(arguments[1]->integer("value") == 22);
    CHECK(context.end());
}

TEST(ParsingHelpers, ParseExpressionListRejectsTrailingComma) {
    ParserRegistry registry{};
    installIntegerPrefix(registry);
    ParserContext context{tokens({
        tok(Kind::Symbol, "("),
        tok(Kind::Integer, "42"),
        tok(Kind::Symbol, ","),
        tok(Kind::Symbol, ")"),
        endTok()
    }), registry};

    CHECK(throwsRuntimeError([&]() {(void)novac::assets::essentials::helpers::parseExpressionList(context, "expr", "(", ")", ",");}));
}

} // namespace
