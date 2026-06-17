#include "../../tester.hpp"
#include "../include/novac/engine/syntax/Lexer.hpp"

#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using novac::lexer::Lexer;
using novac::lexer::LexerRegistry;
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

void checkToken(const Token &token, Kind expectedKind, const std::string &expectedText, int expectedLine, int expectedColumn) {
    CHECK(token.kind == expectedKind);
    CHECK(token.text == expectedText);
    CHECK(token.line == expectedLine);
    CHECK(token.column == expectedColumn);
    CHECK(token.span.begin.line == expectedLine);
    CHECK(token.span.begin.column == expectedColumn);
}

} // namespace

TEST(LexerRegistry, HasDefaultRules) {
    LexerRegistry registry{};

    CHECK(!registry.isKeyword("if"));
    CHECK(registry.symbols().empty());

    CHECK(registry.isIdentifierStart(static_cast<unsigned char>('a')));
    CHECK(registry.isIdentifierStart(static_cast<unsigned char>('_')));
    CHECK(!registry.isIdentifierStart(static_cast<unsigned char>('1')));

    CHECK(registry.isIdentifierContinue(static_cast<unsigned char>('a')));
    CHECK(registry.isIdentifierContinue(static_cast<unsigned char>('_')));
    CHECK(registry.isIdentifierContinue(static_cast<unsigned char>('1')));
    CHECK(!registry.isIdentifierContinue(static_cast<unsigned char>('-')));

    CHECK(registry.lineCommentPrefix() == "//");
    CHECK(registry.blockCommentBegin() == "/*");
    CHECK(registry.blockCommentEnd() == "*/");
}

TEST(LexerRegistry, RegistersKeywords) {
    LexerRegistry registry{};

    CHECK(registry.keyword("if") == RegisterStatus::Inserted);
    CHECK(registry.isKeyword("if"));
    CHECK(!registry.isKeyword("else"));
}

TEST(LexerRegistry, IgnoresDuplicateKeywords) {
    LexerRegistry registry{};

    CHECK(registry.keyword("if") == RegisterStatus::Inserted);
    CHECK(registry.keyword("if") == RegisterStatus::Ignored);
    CHECK(registry.isKeyword("if"));
}

TEST(LexerRegistry, RejectsEmptyKeyword) {
    LexerRegistry registry{};

    CHECK(throwsRuntimeError([&]() { registry.keyword(""); }));
}

TEST(LexerRegistry, RegistersSymbolsWithLongestMatchFirst) {
    LexerRegistry registry{};

    CHECK(registry.symbol("=") == RegisterStatus::Inserted);
    CHECK(registry.symbol("==") == RegisterStatus::Inserted);
    CHECK(registry.symbol("=>") == RegisterStatus::Inserted);
    CHECK(registry.symbol("+") == RegisterStatus::Inserted);

    const std::vector<std::string> &symbols{registry.symbols()};

    CHECK(symbols.size() == 4);
    CHECK(symbols[0] == "==");
    CHECK(symbols[1] == "=>");
    CHECK(symbols[2] == "+");
    CHECK(symbols[3] == "=");
}

TEST(LexerRegistry, IgnoresDuplicateSymbols) {
    LexerRegistry registry{};

    CHECK(registry.symbol("+") == RegisterStatus::Inserted);
    CHECK(registry.symbol("+") == RegisterStatus::Ignored);
    CHECK(registry.symbols().size() == 1);
}

TEST(LexerRegistry, RejectsEmptySymbol) {
    LexerRegistry registry{};

    CHECK(throwsRuntimeError([&]() {registry.symbol("");}));
}

TEST(LexerRegistry, CustomizesIdentifierRules) {
    LexerRegistry registry{};

    registry.setIdentifierRules(
        [](unsigned char value) { return value == '@'; },
        [](unsigned char value) { return value == '@' || value == '-' || value == 'x'; }
    );

    CHECK(registry.isIdentifierStart(static_cast<unsigned char>('@')));
    CHECK(!registry.isIdentifierStart(static_cast<unsigned char>('a')));

    CHECK(registry.isIdentifierContinue(static_cast<unsigned char>('@')));
    CHECK(registry.isIdentifierContinue(static_cast<unsigned char>('-')));
    CHECK(registry.isIdentifierContinue(static_cast<unsigned char>('x')));
    CHECK(!registry.isIdentifierContinue(static_cast<unsigned char>('a')));
}

TEST(LexerRegistry, RejectsEmptyIdentifierPredicates) {
    LexerRegistry registry{};

    CHECK(throwsRuntimeError([&]() {registry.setIdentifierRules({}, [](unsigned char) { return true; });}));
    CHECK(throwsRuntimeError([&]() {registry.setIdentifierRules([](unsigned char) { return true; }, {});}));
}

TEST(LexerRegistry, ConfiguresComments) {
    LexerRegistry registry{};

    registry.setLineCommentPrefix("#");
    CHECK(registry.lineCommentPrefix() == "#");

    registry.setBlockCommentDelimiters("<#", "#>");
    CHECK(registry.blockCommentBegin() == "<#");
    CHECK(registry.blockCommentEnd() == "#>");

    registry.setBlockCommentDelimiters("", "");
    CHECK(registry.blockCommentBegin().empty());
    CHECK(registry.blockCommentEnd().empty());
}

TEST(LexerRegistry, RejectsPartialBlockCommentDelimiters) {
    LexerRegistry registry{};

    CHECK(throwsRuntimeError([&]() {registry.setBlockCommentDelimiters("/*", "");}));
    CHECK(throwsRuntimeError([&]() {registry.setBlockCommentDelimiters("", "*/");}));
}

TEST(Lexer, CreatesEndTokenForEmptySource) {
    LexerRegistry registry{};
    Lexer lexer{registry};

    const std::vector<Token> tokens{lexer.tokenize("")};

    CHECK(tokens.size() == 1);
    checkToken(tokens[0], Kind::End, "", 1, 1);
}

TEST(Lexer, TokenizesIdentifiersAndKeywords) {
    LexerRegistry registry{};
    registry.keyword("if");
    registry.keyword("return");

    Lexer lexer{registry};
    const std::vector<Token> tokens{lexer.tokenize("if value return")};

    CHECK(tokens.size() == 4);
    checkToken(tokens[0], Kind::Keyword, "if", 1, 1);
    checkToken(tokens[1], Kind::Identifier, "value", 1, 4);
    checkToken(tokens[2], Kind::Keyword, "return", 1, 10);
    checkToken(tokens[3], Kind::End, "", 1, 16);
}

TEST(Lexer, TokenizesSymbolsUsingLongestMatch) {
    LexerRegistry registry{};
    registry.symbol("=");
    registry.symbol("==");
    registry.symbol("+");

    Lexer lexer{registry};
    const std::vector<Token> tokens{lexer.tokenize("a==b + c")};

    CHECK(tokens.size() == 6);
    checkToken(tokens[0], Kind::Identifier, "a", 1, 1);
    checkToken(tokens[1], Kind::Symbol, "==", 1, 2);
    checkToken(tokens[2], Kind::Identifier, "b", 1, 4);
    checkToken(tokens[3], Kind::Symbol, "+", 1, 6);
    checkToken(tokens[4], Kind::Identifier, "c", 1, 8);
    checkToken(tokens[5], Kind::End, "", 1, 9);
}

TEST(Lexer, TokenizesIntegerAndFloatLiterals) {
    LexerRegistry registry{};
    Lexer lexer{registry};

    const std::vector<Token> tokens{lexer.tokenize("123 45.67 8")};

    CHECK(tokens.size() == 4);
    checkToken(tokens[0], Kind::Integer, "123", 1, 1);
    CHECK(tokens[0].suffix.empty());

    checkToken(tokens[1], Kind::Float, "45.67", 1, 5);
    CHECK(tokens[1].suffix.empty());

    checkToken(tokens[2], Kind::Integer, "8", 1, 11);
    CHECK(tokens[2].suffix.empty());

    checkToken(tokens[3], Kind::End, "", 1, 12);
}

TEST(Lexer, TokenizesLiteralSuffixes) {
    LexerRegistry registry{};
    Lexer lexer{registry};

    const std::vector<Token> tokens{lexer.tokenize("42_i32 3.14_f64 \"abc\"_str")};

    CHECK(tokens.size() == 4);

    checkToken(tokens[0], Kind::Integer, "42", 1, 1);
    CHECK(tokens[0].suffix == "_i32");

    checkToken(tokens[1], Kind::Float, "3.14", 1, 8);
    CHECK(tokens[1].suffix == "_f64");

    checkToken(tokens[2], Kind::String, "abc", 1, 17);
    CHECK(tokens[2].suffix == "_str");

    checkToken(tokens[3], Kind::End, "", 1, 26);
}

TEST(Lexer, RejectsInvalidLiteralSuffix) {
    LexerRegistry registry{};
    Lexer lexer{registry};

    CHECK(throwsRuntimeError([&]() {lexer.tokenize("42_");}));
    CHECK(throwsRuntimeError([&]() {lexer.tokenize("42_1");}));
}

TEST(Lexer, TokenizesStringLiterals) {
    LexerRegistry registry{};
    Lexer lexer{registry};

    const std::vector<Token> tokens{lexer.tokenize("\"hello\" \"a\\nb\" \"quote: \\\"\"")};

    CHECK(tokens.size() == 4);
    checkToken(tokens[0], Kind::String, "hello", 1, 1);
    checkToken(tokens[1], Kind::String, std::string{"a\nb"}, 1, 9);
    checkToken(tokens[2], Kind::String, "quote: \"", 1, 16);
    checkToken(tokens[3], Kind::End, "", 1, 27);
}

TEST(Lexer, RejectsInvalidStrings) {
    LexerRegistry registry{};
    Lexer lexer{registry};

    CHECK(throwsRuntimeError([&]() {lexer.tokenize("\"unterminated");}));
    CHECK(throwsRuntimeError([&]() {lexer.tokenize("\"line\nbreak\"");}));
    CHECK(throwsRuntimeError([&]() {lexer.tokenize("\"bad\\xescape\"");}));
    CHECK(throwsRuntimeError([&]() {lexer.tokenize("\"bad\\");}));
}

TEST(Lexer, SkipsDefaultComments) {
    LexerRegistry registry{};
    registry.symbol("+");

    Lexer lexer{registry};
    const std::vector<Token> tokens{lexer.tokenize("a // ignored\n/* also ignored */ b + c")};

    CHECK(tokens.size() == 5);
    checkToken(tokens[0], Kind::Identifier, "a", 1, 1);
    checkToken(tokens[1], Kind::Identifier, "b", 2, 20);
    checkToken(tokens[2], Kind::Symbol, "+", 2, 22);
    checkToken(tokens[3], Kind::Identifier, "c", 2, 24);
    checkToken(tokens[4], Kind::End, "", 2, 25);
}

TEST(Lexer, SupportsCustomComments) {
    LexerRegistry registry{};
    registry.setLineCommentPrefix("#");
    registry.setBlockCommentDelimiters("<#", "#>");

    Lexer lexer{registry};
    const std::vector<Token> tokens{lexer.tokenize("a # ignored\n<# ignored #> b")};

    CHECK(tokens.size() == 3);
    checkToken(tokens[0], Kind::Identifier, "a", 1, 1);
    checkToken(tokens[1], Kind::Identifier, "b", 2, 15);
    checkToken(tokens[2], Kind::End, "", 2, 16);
}

TEST(Lexer, RejectsUnterminatedBlockComment) {
    LexerRegistry registry{};
    Lexer lexer{registry};

    CHECK(throwsRuntimeError([&]() {lexer.tokenize("a /* missing end");}));
}

TEST(Lexer, TracksSourceLocationsAndFileNames) {
    LexerRegistry registry{};
    Lexer lexer{registry};

    const std::vector<Token> tokens{lexer.tokenize("first\n  second", "sample.novac")};

    CHECK(tokens.size() == 3);

    checkToken(tokens[0], Kind::Identifier, "first", 1, 1);
    CHECK(tokens[0].span.begin.file == "sample.novac");
    CHECK(tokens[0].span.begin.offset == 0);
    CHECK(tokens[0].span.end.offset == 5);

    checkToken(tokens[1], Kind::Identifier, "second", 2, 3);
    CHECK(tokens[1].span.begin.file == "sample.novac");
    CHECK(tokens[1].span.begin.offset == 8);
    CHECK(tokens[1].span.end.offset == 14);

    checkToken(tokens[2], Kind::End, "", 2, 9);
    CHECK(tokens[2].span.begin.file == "sample.novac");
}

TEST(Lexer, UsesCustomIdentifierRulesDuringTokenization) {
    LexerRegistry registry{};
    registry.setIdentifierRules(
        [](unsigned char value) { return value == '@';},
        [](unsigned char value) {return value == '@' || value == 'x' || value == '-';});

    Lexer lexer{registry};
    const std::vector<Token> tokens{lexer.tokenize("@x-x")};

    CHECK(tokens.size() == 2);
    checkToken(tokens[0], Kind::Identifier, "@x-x", 1, 1);
    checkToken(tokens[1], Kind::End, "", 1, 5);
}

TEST(Lexer, RejectsUnexpectedCharacters) {
    LexerRegistry registry{};
    Lexer lexer{registry};

    CHECK(throwsRuntimeError([&]() {lexer.tokenize("$");}));
}
