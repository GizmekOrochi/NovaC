#include "../../../tester.hpp"
#include "novac/assets/atomic/literals/IntegerLiteralAtomic.hpp"
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

using novac::assets::atomic::literals::IntegerLiteralAtomic;
using novac::assets::atomic::LiteralInfo;
using novac::assets::atomic::TokenPattern;
using novac::assets::atomic::TokenPatternMode;

TEST(IntegerLiteralAtomic, DefaultConstruction) {
    IntegerLiteralAtomic feature;
    CHECK(true);
}

TEST(IntegerLiteralAtomic, CustomConstruction) {
    IntegerLiteralAtomic feature("CustomInt", TokenPattern::text("42"));
    CHECK(true);
}

TEST(IntegerLiteralAtomic, ConstructionWithTokenKeyPattern) {
    IntegerLiteralAtomic feature("MyInt", TokenPattern::key("$int"));
    CHECK(true);
}

TEST(IntegerLiteralAtomic, ConstructionWithSuffixPattern) {
    IntegerLiteralAtomic feature("SuffixedInt", TokenPattern::suffixed("int", "i64"));
    CHECK(true);
}

TEST(IntegerLiteralAtomic, ConstructionWithSuffixRegexPattern) {
    IntegerLiteralAtomic feature("RegexInt", TokenPattern::suffixRegex("int", "^[iI][0-9]+$"));
    CHECK(true);
}

TEST(IntegerLiteralAtomic, EmptyNodeKindThrows) {
    CHECK(throwsRuntimeError([]() { IntegerLiteralAtomic feature(""); }));
}

TEST(IntegerLiteralAtomic, InfoReturnsCorrectDefaultMetadata) {
    IntegerLiteralAtomic feature;
    LiteralInfo info = feature.info();

    CHECK(info.id == "core.literal.integer");
    CHECK(info.version == "0.1.0");
    CHECK(info.description == "Integer literal atomic");
    CHECK(info.nodeKind == "IntegerLiteral");
    CHECK(info.pattern.mode == TokenPatternMode::TokenKey);
    CHECK(info.pattern.tokenKey == "$int");
    CHECK(info.capabilities.size() == 2);
    CHECK(info.capabilities[0] == "literal.integer");
    CHECK(info.capabilities[1] == "expression.atom");
    CHECK(info.requiredCapabilities.empty());
}

TEST(IntegerLiteralAtomic, InfoReturnsCustomMetadata) {
    IntegerLiteralAtomic feature("MyCustomInt", TokenPattern::text("123"));
    LiteralInfo info = feature.info();

    CHECK(info.nodeKind == "MyCustomInt");
    CHECK(info.pattern.mode == TokenPatternMode::Text);
    CHECK(info.pattern.token == "123");
}

TEST(IntegerLiteralAtomic, InfoWithSuffixPattern) {
    IntegerLiteralAtomic feature("SuffixedInt", TokenPattern::suffixed("int", "i64"));
    LiteralInfo info = feature.info();

    CHECK(info.pattern.mode == TokenPatternMode::Suffix);
    CHECK(info.pattern.tokenKey == "int");
    CHECK(info.pattern.suffix == "i64");
}

TEST(IntegerLiteralAtomic, InfoWithSuffixRegexPattern) {
    IntegerLiteralAtomic feature("RegexInt", TokenPattern::suffixRegex("int", "^[iI][0-9]+$"));
    LiteralInfo info = feature.info();

    CHECK(info.pattern.mode == TokenPatternMode::SuffixRegex);
    CHECK(info.pattern.tokenKey == "int");
    CHECK(info.pattern.suffixPattern == "^[iI][0-9]+$");
}

} // namespace