#include "../../tester.hpp"
#include "novac/assets/atomic/AtomicPattern.hpp"

namespace {

using novac::assets::atomic::TokenPattern;
using novac::assets::atomic::TokenPatternMode;

TEST(AtomicPattern, DefaultConstruction) {
    TokenPattern pattern{};
    CHECK(pattern.mode == TokenPatternMode::Text);
    CHECK(pattern.token.empty());
    CHECK(pattern.tokenKey.empty());
    CHECK(pattern.suffix.empty());
    CHECK(pattern.suffixPattern.empty());
    CHECK(!pattern.keyword);
}

TEST(AtomicPattern, TextPattern) {
    TokenPattern pattern = TokenPattern::text("if");
    CHECK(pattern.mode == TokenPatternMode::Text);
    CHECK(pattern.token == "if");
    CHECK(pattern.tokenKey.empty());
    CHECK(pattern.suffix.empty());
    CHECK(pattern.suffixPattern.empty());
    CHECK(!pattern.keyword);
}

TEST(AtomicPattern, KeyPattern) {
    TokenPattern pattern = TokenPattern::key("identifier");
    CHECK(pattern.mode == TokenPatternMode::TokenKey);
    CHECK(pattern.tokenKey == "identifier");
    CHECK(pattern.token.empty());
    CHECK(pattern.suffix.empty());
    CHECK(pattern.suffixPattern.empty());
    CHECK(!pattern.keyword);
}

TEST(AtomicPattern, KeywordTextPattern) {
    TokenPattern pattern = TokenPattern::keywordText("return");
    CHECK(pattern.mode == TokenPatternMode::Text);
    CHECK(pattern.token == "return");
    CHECK(pattern.tokenKey.empty());
    CHECK(pattern.suffix.empty());
    CHECK(pattern.suffixPattern.empty());
    CHECK(pattern.keyword);
}

TEST(AtomicPattern, SuffixedPattern) {
    TokenPattern pattern = TokenPattern::suffixed("number", "int");
    CHECK(pattern.mode == TokenPatternMode::Suffix);
    CHECK(pattern.tokenKey == "number");
    CHECK(pattern.suffix == "int");
    CHECK(pattern.token.empty());
    CHECK(pattern.suffixPattern.empty());
    CHECK(!pattern.keyword);
}

TEST(AtomicPattern, SuffixRegexPattern) {
    TokenPattern pattern = TokenPattern::suffixRegex("number", "^[0-9]+$");
    CHECK(pattern.mode == TokenPatternMode::SuffixRegex);
    CHECK(pattern.tokenKey == "number");
    CHECK(pattern.suffixPattern == "^[0-9]+$");
    CHECK(pattern.token.empty());
    CHECK(pattern.suffix.empty());
    CHECK(!pattern.keyword);
}

TEST(AtomicPattern, MoveSemanticsForText) {
    std::string value = "test";
    TokenPattern pattern = TokenPattern::text(std::move(value));
    CHECK(pattern.token == "test");
    CHECK(value.empty()); // Vérifie que std::move a bien été utilisé
}

TEST(AtomicPattern, MoveSemanticsForKey) {
    std::string value = "test_key";
    TokenPattern pattern = TokenPattern::key(std::move(value));
    CHECK(pattern.tokenKey == "test_key");
    CHECK(value.empty());
}

TEST(AtomicPattern, MoveSemanticsForSuffixed) {
    std::string key = "test_key";
    std::string suffix = "test_suffix";
    TokenPattern pattern = TokenPattern::suffixed(std::move(key), std::move(suffix));
    CHECK(pattern.tokenKey == "test_key");
    CHECK(pattern.suffix == "test_suffix");
    CHECK(key.empty());
    CHECK(suffix.empty());
}

TEST(AtomicPattern, MoveSemanticsForSuffixRegex) {
    std::string key = "test_key";
    std::string regex = "^test$";
    TokenPattern pattern = TokenPattern::suffixRegex(std::move(key), std::move(regex));
    CHECK(pattern.tokenKey == "test_key");
    CHECK(pattern.suffixPattern == "^test$");
    CHECK(key.empty());
    CHECK(regex.empty());
}

} // namespace