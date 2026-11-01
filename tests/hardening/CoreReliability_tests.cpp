#include "../tester.hpp"
#include "novac/engine/EngineController.hpp"
#include "novac/engine/syntax/Parser.hpp"

#include <string>
#include <utility>
#include <vector>

namespace {
using novac::ast::Node;
using novac::ast::NodePtr;
using novac::parser::ParserContext;
using novac::parser::ParserRegistry;
using novac::token::Kind;
using novac::token::Token;

Token tok(Kind kind, std::string text) {
    return Token{kind, std::move(text), "", {}, 1, 1};
}
Token endTok() { return tok(Kind::End, ""); }

TEST(CoreReliability, AdvancingAtEndIsStable) {
    ParserRegistry registry{};
    ParserContext context{{}, registry};

    const Token &first{context.advance()};
    const Token &second{context.advance()};

    CHECK(first.kind == Kind::End);
    CHECK(second.kind == Kind::End);
    CHECK(context.end());
    CHECK(context.cur().kind == Kind::End);
}

TEST(CoreReliability, ConsumingEndTokenIsSafe) {
    ParserRegistry registry{};
    ParserContext context{{}, registry};

    const Token &consumed{context.consumeKind(Kind::End)};

    CHECK(consumed.kind == Kind::End);
    CHECK(context.end());
}

TEST(CoreReliability, FailedFallbackRestoresParserPosition) {
    ParserRegistry registry{};

    registry.fallback("expr", [](ParserContext &context) -> NodePtr {
        context.consumeKind(Kind::Identifier);
        return nullptr;
    });

    registry.fallback("expr", [](ParserContext &context) -> NodePtr {
        const Token token{context.consumeKind(Kind::Identifier)};
        auto node{Node::make("Identifier")};
        node->set("name", token.text);
        return node;
    });

    ParserContext context{{tok(Kind::Identifier, "value"), endTok()}, registry};
    const NodePtr node{registry.parse(context, "expr")};

    CHECK(node != nullptr);
    CHECK(node->kind() == "Identifier");
    CHECK(node->str("name") == "value");
    CHECK(context.end());
}

} // namespace
