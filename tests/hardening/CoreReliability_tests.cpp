#include "../tester.hpp"

#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/assets/atomic/LiteralFeature.hpp"
#include "novac/assets/atomic/operations/NumericOperations.hpp"
#include "novac/assets/essentials/EssentialFeature.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/engine/EngineController.hpp"
#include "novac/engine/syntax/Parser.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
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

using novac::assets::atomic::AtomicController;
using novac::assets::atomic::LiteralInfo;
using novac::assets::atomic::TokenPattern;
using novac::assets::atomic::operations::AddOperationAtomic;
using novac::assets::essentials::EssentialInfo;
using novac::assets::essentials::EssentialsController;
using novac::ast::Node;
using novac::ast::NodePtr;
using novac::controllers::EngineController;
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

class FailingAtomicLiteral final : public novac::assets::atomic::LiteralFeature {
public:
    LiteralInfo info() const override {
        return {
            "hardening.atomic.failure",
            "0.1.0",
            "Hardening feature that intentionally fails",
            "HardeningAtomicLiteral",
            TokenPattern::keywordText("hard_atomic"),
            {"hardening.atomic.capability"},
            {}
        };
    }

    void install(AtomicController &controller) const override {
        controller.registerPattern(info().pattern);
        controller.engine().keyword("hard_atomic_keyword");
        controller.engine().registerCapability("hardening.atomic.engine");
        throw std::runtime_error("intentional atomic installation failure");
    }
};

class FailingEssentialFeature final : public novac::assets::essentials::EssentialFeature {
public:
    EssentialInfo info() const override {
        return {
            "hardening.essentials.failure",
            "0.1.0",
            "Hardening feature that intentionally fails",
            {},
            {},
            {"hardening.essentials.capability"},
            {}
        };
    }

    void install(EssentialsController &controller) const override {
        controller.engine().keyword("hard_essential_keyword");
        controller.engine().registerCapability("hardening.essentials.engine");
        controller.functionRegistry().native(
            "hardening_native",
            [](const novac::ast::NodeList &, novac::runtime::RuntimeContext &) {
                return novac::runtime::Value::integer(1);
            }
        );
        throw std::runtime_error("intentional Essentials installation failure");
    }
};

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

TEST(CoreReliability, TemporaryAtomicFeatureRemainsUsable) {
    EngineController engine{};
    AtomicController atomics{engine};

    atomics.integer();
    atomics.use(AddOperationAtomic{});

    const auto expression{engine.parse("20 + 22")};
    CHECK(engine.eval(*expression).asInt() == 42);
}

TEST(CoreReliability, FunctionArgumentsUseCallerScopeBeforeParameterBinding) {
    EngineController engine{};
    AtomicController atomics{engine};
    atomics.integer();

    EssentialsController essentials{engine};
    essentials.installStandardCore();

    const auto program{engine.parse(R"(
func select(first, second) {
    return second;
}

func main() {
    let first = 42;
    return select(1, first);
}
)")};

    CHECK(engine.eval(*program).asInt() == 42);
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

TEST(CoreReliability, FailedAtomicInstallLeavesNoPartialState) {
    EngineController engine{};
    AtomicController atomics{engine};

    CHECK(throwsRuntimeError([&]() {
        atomics.use(FailingAtomicLiteral{});
    }));

    CHECK(!atomics.hasLiteral("hardening.atomic.failure"));
    CHECK(!engine.hasCapability("hardening.atomic.engine"));
    CHECK(!engine.hasCapability("hardening.atomic.capability"));

    const auto tokens{engine.tokenize("hard_atomic_keyword")};
    CHECK(tokens[0].kind == Kind::Identifier);
}

TEST(CoreReliability, FailedEssentialsInstallLeavesNoPartialState) {
    EngineController engine{};
    EssentialsController essentials{engine};

    CHECK(throwsRuntimeError([&]() {
        essentials.use(FailingEssentialFeature{});
    }));

    CHECK(!essentials.hasFeature("hardening.essentials.failure"));
    CHECK(!engine.hasCapability("hardening.essentials.engine"));
    CHECK(!engine.hasCapability("hardening.essentials.capability"));
    CHECK(!essentials.functionRegistry().hasNative("hardening_native"));

    const auto tokens{engine.tokenize("hard_essential_keyword")};
    CHECK(tokens[0].kind == Kind::Identifier);
}


TEST(CoreReliability, AtomicCallbacksSurviveControllerDestruction) {
    EngineController engine{};

    {
        AtomicController atomics{engine};
        atomics.integer();
        atomics.use(AddOperationAtomic{});
    }

    const auto expression{engine.parse("20 + 22")};
    CHECK(engine.eval(*expression).asInt() == 42);
}

TEST(CoreReliability, EssentialsCallbacksSurviveControllerDestruction) {
    EngineController engine{};

    {
        AtomicController atomics{engine};
        atomics.integer();
    }

    {
        EssentialsController essentials{engine};
        essentials.installStandardCore();
        essentials.functionRegistry().native(
            "answer",
            [](const novac::ast::NodeList &, novac::runtime::RuntimeContext &) {
                return novac::runtime::Value::integer(42);
            }
        );
    }

    const auto program{engine.parse(R"(
func main() {
    return answer();
}
)")};

    CHECK(engine.eval(*program).asInt() == 42);
}

} // namespace
