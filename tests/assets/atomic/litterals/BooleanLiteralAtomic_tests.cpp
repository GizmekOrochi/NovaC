#include "../../../tester.hpp"
#include "novac/assets/atomic/literals/BooleanLiteralAtomic.hpp"
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

using novac::assets::atomic::literals::BooleanLiteralAtomic;
using novac::assets::atomic::literals::BooleanLiteralTokens;
using novac::assets::atomic::LiteralInfo;
using novac::assets::atomic::TokenPatternMode;

TEST(BooleanLiteralAtomic, DefaultConstruction) {
    BooleanLiteralAtomic feature;
    CHECK(true);
}

TEST(BooleanLiteralAtomic, CustomConstruction) {
    BooleanLiteralTokens tokens{"yes", "no"};
    BooleanLiteralAtomic feature("CustomBoolean", tokens);
    CHECK(true);
}

TEST(BooleanLiteralAtomic, EmptyNodeKindThrows) {
    CHECK(throwsRuntimeError([]() { BooleanLiteralAtomic feature("", BooleanLiteralTokens{"true", "false"}); }));
}

TEST(BooleanLiteralAtomic, EmptyTrueTokenThrows) {
    CHECK(throwsRuntimeError([]() { BooleanLiteralAtomic feature("BooleanLiteral", BooleanLiteralTokens{"", "false"}); }));
}

TEST(BooleanLiteralAtomic, EmptyFalseTokenThrows) {
    CHECK(throwsRuntimeError([]() { BooleanLiteralAtomic feature("BooleanLiteral", BooleanLiteralTokens{"true", ""}); }));
}

TEST(BooleanLiteralAtomic, InfoReturnsCorrectDefaultMetadata) {
    BooleanLiteralAtomic feature;
    LiteralInfo info = feature.info();

    CHECK(info.id == "core.literal.boolean");
    CHECK(info.version == "0.1.0");
    CHECK(info.description == "Boolean literal atomic");
    CHECK(info.nodeKind == "BooleanLiteral");
    CHECK(info.pattern.mode == TokenPatternMode::Text);
    CHECK(info.pattern.token == "true");
    CHECK(info.pattern.keyword == true);
    CHECK(info.capabilities.size() == 2);
    CHECK(info.capabilities[0] == "literal.boolean");
    CHECK(info.capabilities[1] == "expression.atom");
    CHECK(info.requiredCapabilities.empty());
}

TEST(BooleanLiteralAtomic, InfoReturnsCustomMetadata) {
    BooleanLiteralTokens tokens{"oui", "non"};
    BooleanLiteralAtomic feature("MonBooléen", tokens);
    LiteralInfo info = feature.info();

    CHECK(info.nodeKind == "MonBooléen");
    CHECK(info.pattern.token == "oui");
}

} // namespace