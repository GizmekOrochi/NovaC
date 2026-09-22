#include "../../../tester.hpp"
#include "novac/assets/atomic/literals/StringLiteralAtomic.hpp"
#include "novac/assets/atomic/AtomicPattern.hpp"

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

using novac::assets::atomic::literals::StringLiteralAtomic;
using novac::assets::atomic::LiteralInfo;
using novac::assets::atomic::TokenPattern;
using novac::assets::atomic::TokenPatternMode;

TEST(StringLiteralAtomic, DefaultConstruction) {
    StringLiteralAtomic feature;
    CHECK(true);
}

TEST(StringLiteralAtomic, CustomConstruction) {
    StringLiteralAtomic feature("CustomString", TokenPattern::text("\"hello\""));
    CHECK(true);
}

TEST(StringLiteralAtomic, EmptyNodeKindThrows) {
    CHECK(throwsRuntimeError([]() { StringLiteralAtomic feature(""); }));
}

TEST(StringLiteralAtomic, InfoReturnsCorrectDefaultMetadata) {
    StringLiteralAtomic feature;
    LiteralInfo info = feature.info();

    CHECK(info.id == "core.literal.string");
    CHECK(info.version == "0.1.0");
    CHECK(info.description == "String literal atomic");
    CHECK(info.nodeKind == "StringLiteral");
    CHECK(info.pattern.mode == TokenPatternMode::TokenKey);
    CHECK(info.pattern.tokenKey == "$string");
    CHECK(info.capabilities.size() == 2);
    CHECK(info.capabilities[0] == "literal.string");
    CHECK(info.capabilities[1] == "expression.atom");
    CHECK(info.requiredCapabilities.empty());
}

TEST(StringLiteralAtomic, InfoReturnsCustomMetadata) {
    StringLiteralAtomic feature("MyString", TokenPattern::text("\"test\""));
    LiteralInfo info = feature.info();

    CHECK(info.nodeKind == "MyString");
    CHECK(info.pattern.mode == TokenPatternMode::Text);
    CHECK(info.pattern.token == "\"test\"");
}

TEST(StringLiteralAtomic, InfoWithSuffixPattern) {
    StringLiteralAtomic feature("SuffixedString", TokenPattern::suffixed("str", "s"));
    LiteralInfo info = feature.info();

    CHECK(info.pattern.mode == TokenPatternMode::Suffix);
    CHECK(info.pattern.tokenKey == "str");
    CHECK(info.pattern.suffix == "s");
}

TEST(StringLiteralAtomic, InfoWithSuffixRegexPattern) {
    StringLiteralAtomic feature("RegexString", TokenPattern::suffixRegex("str", "^[sS]$"));
    LiteralInfo info = feature.info();

    CHECK(info.pattern.mode == TokenPatternMode::SuffixRegex);
    CHECK(info.pattern.tokenKey == "str");
    CHECK(info.pattern.suffixPattern == "^[sS]$");
}

} // namespace