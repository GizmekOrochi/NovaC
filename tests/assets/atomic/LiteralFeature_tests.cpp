#include "../../tester.hpp"
#include "novac/assets/atomic/LiteralFeature.hpp"
#include "novac/assets/atomic/AtomicPattern.hpp"

namespace {

using novac::assets::atomic::LiteralFeature;
using novac::assets::atomic::LiteralInfo;
using novac::assets::atomic::TokenPattern;
using novac::assets::atomic::TokenPatternMode;
using novac::assets::atomic::AtomicController;

namespace test {
    // Implémentation concrète pour les tests
    class IntegerLiteralFeature : public LiteralFeature {
    public:
        LiteralInfo info() const override {
            return LiteralInfo{
                "integer_literal",
                "1.0.0",
                "Represents integer literals like 42 or 0xFF",
                "IntegerLiteral",
                TokenPattern::text("42"),
                {"literal", "integer"},
                {"lexer"}
            };
        }

        void install(AtomicController &controller) const override {
            (void)controller;
        }
    };

    class KeywordLiteralFeature : public LiteralFeature {
    public:
        LiteralInfo info() const override {
            return LiteralInfo{
                "true_keyword",
                "0.2.0",
                "Boolean true literal",
                "BooleanLiteral",
                TokenPattern::keywordText("true"),
                {"literal", "boolean"},
                {"lexer"}
            };
        }

        void install(AtomicController &controller) const override {
            (void)controller;
        }
    };
} // namespace test

TEST(LiteralFeature, InfoReturnsCorrectDefaultMetadata) {
    test::IntegerLiteralFeature feature;
    LiteralInfo info = feature.info();

    CHECK(info.id == "integer_literal");
    CHECK(info.version == "1.0.0");
    CHECK(info.description == "Represents integer literals like 42 or 0xFF");
    CHECK(info.nodeKind == "IntegerLiteral");
    CHECK(info.pattern.mode == TokenPatternMode::Text);
    CHECK(info.pattern.token == "42");
    CHECK(!info.pattern.keyword);
    CHECK(info.capabilities.size() == 2);
    CHECK(info.capabilities[0] == "literal");
    CHECK(info.capabilities[1] == "integer");
    CHECK(info.requiredCapabilities.size() == 1);
    CHECK(info.requiredCapabilities[0] == "lexer");
}

TEST(LiteralFeature, InfoHandlesKeywordPattern) {
    test::KeywordLiteralFeature feature;
    LiteralInfo info = feature.info();

    CHECK(info.id == "true_keyword");
    CHECK(info.pattern.mode == TokenPatternMode::Text);
    CHECK(info.pattern.token == "true");
    CHECK(info.pattern.keyword == true);
}

} // namespace