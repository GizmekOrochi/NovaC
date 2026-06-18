#include "../../../tester.hpp"
#include "novac/assets/atomic/literals/FloatLiteralAtomic.hpp"
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

using novac::assets::atomic::literals::FloatLiteralAtomic;
using novac::assets::atomic::LiteralInfo;
using novac::assets::atomic::TokenPattern;
using novac::assets::atomic::TokenPatternMode;

TEST(FloatLiteralAtomic, DefaultConstruction) {
    FloatLiteralAtomic feature;
    CHECK(true);
}

TEST(FloatLiteralAtomic, CustomConstruction) {
    FloatLiteralAtomic feature("CustomFloat", TokenPattern::text("3.14"));
    CHECK(true);
}

TEST(FloatLiteralAtomic, EmptyNodeKindThrows) {
    CHECK(throwsRuntimeError([]() { FloatLiteralAtomic feature(""); }));
}

TEST(FloatLiteralAtomic, InfoReturnsCorrectDefaultMetadata) {
    FloatLiteralAtomic feature;
    LiteralInfo info = feature.info();

    CHECK(info.id == "core.literal.float");
    CHECK(info.version == "0.1.0");
    CHECK(info.description == "Floating-point literal atomic");
    CHECK(info.nodeKind == "FloatLiteral");
    CHECK(info.pattern.mode == TokenPatternMode::TokenKey);
    CHECK(info.pattern.tokenKey == "$float");
    CHECK(info.capabilities.size() == 2);
    CHECK(info.capabilities[0] == "literal.float");
    CHECK(info.capabilities[1] == "expression.atom");
    CHECK(info.requiredCapabilities.empty());
}

TEST(FloatLiteralAtomic, InfoReturnsCustomMetadata) {
    FloatLiteralAtomic feature("MyFloat", TokenPattern::text("1.23"));
    LiteralInfo info = feature.info();

    CHECK(info.nodeKind == "MyFloat");
    CHECK(info.pattern.mode == TokenPatternMode::Text);
    CHECK(info.pattern.token == "1.23");
}

TEST(FloatLiteralAtomic, InfoWithSuffixPattern) {
    FloatLiteralAtomic feature("SuffixedFloat", TokenPattern::suffixed("float", "f32"));
    LiteralInfo info = feature.info();

    CHECK(info.pattern.mode == TokenPatternMode::Suffix);
    CHECK(info.pattern.tokenKey == "float");
    CHECK(info.pattern.suffix == "f32");
}

TEST(FloatLiteralAtomic, InfoWithSuffixRegexPattern) {
    FloatLiteralAtomic feature("RegexFloat", TokenPattern::suffixRegex("float", "^[fF][0-9]+$"));
    LiteralInfo info = feature.info();

    CHECK(info.pattern.mode == TokenPatternMode::SuffixRegex);
    CHECK(info.pattern.tokenKey == "float");
    CHECK(info.pattern.suffixPattern == "^[fF][0-9]+$");
}

} // namespace