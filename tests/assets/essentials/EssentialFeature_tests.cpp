#include "../../tester.hpp"

#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/assets/essentials/EssentialFeature.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/engine/EngineController.hpp"

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

namespace test {

class EmptyIdFeature final : public novac::assets::essentials::EssentialFeature {
public:
    novac::assets::essentials::EssentialInfo info() const override {
        return {};
    }

    void install(novac::assets::essentials::EssentialsController &) const override {
    }
};

class TestFeature final : public novac::assets::essentials::EssentialFeature {
public:
    novac::assets::essentials::EssentialInfo info() const override {
        return {"test.essential", "1.0.0", "Test essential feature", {"TestNode"}, {"test.trait"}, {"test.capability"}, {}};
    }

    void install(novac::assets::essentials::EssentialsController &) const override {
    }
};

class RequiresCapabilityFeature final : public novac::assets::essentials::EssentialFeature {
public:
    novac::assets::essentials::EssentialInfo info() const override {
        return {"test.consumer", "1.0.0", "Capability consumer", {}, {}, {"test.consumer"}, {"test.capability"}};
    }

    void install(novac::assets::essentials::EssentialsController &) const override {
    }
};

class RequiresAtomicExpressionFeature final : public novac::assets::essentials::EssentialFeature {
public:
    novac::assets::essentials::EssentialInfo info() const override {
        return {"test.atomic-consumer", "1.0.0", "Atomic capability consumer", {}, {}, {"test.atomic-consumer"}, {"expression.atom"}};
    }

    void install(novac::assets::essentials::EssentialsController &) const override {
    }
};

class FailingTransactionalFeature final : public novac::assets::essentials::EssentialFeature {
public:
    novac::assets::essentials::EssentialInfo info() const override {
        return {
            "test.transaction.failure",
            "1.0.0",
            "Feature that fails after mutating Essentials and Engine state",
            {},
            {},
            {"test.transaction.failure"},
            {}
        };
    }

    void install(novac::assets::essentials::EssentialsController &controller) const override {
        controller.engine().keyword("transaction_keyword");
        controller.engine().registerCapability("test.transaction.engine");
        controller.functionRegistry().native(
            "transaction_native",
            [](const novac::ast::NodeList &, novac::runtime::RuntimeContext &) {
                return novac::runtime::Value::integer(1);
            }
        );
        controller.own(std::make_unique<TestFeature>());

        throw std::runtime_error("FailingTransactionalFeature::install: intentional failure");
    }
};

} // namespace test

using novac::assets::essentials::EssentialInfo;
using novac::assets::essentials::EssentialPack;
using novac::assets::essentials::EssentialsController;
using novac::controllers::EngineController;

TEST(EssentialFeature, InfoReturnsMetadata) {
    test::TestFeature feature;
    EssentialInfo info{feature.info()};

    CHECK(info.id == "test.essential");
    CHECK(info.version == "1.0.0");
    CHECK(info.description == "Test essential feature");
    CHECK(info.nodeKinds.size() == 1);
    CHECK(info.nodeKinds[0] == "TestNode");
    CHECK(info.traits.size() == 1);
    CHECK(info.traits[0] == "test.trait");
    CHECK(info.capabilities.size() == 1);
    CHECK(info.capabilities[0] == "test.capability");
    CHECK(info.requirements.empty());
}

TEST(EssentialFeature, MissingCapabilityIsRejected) {
    EngineController engine{};
    EssentialsController controller{engine};
    test::RequiresCapabilityFeature feature{};

    CHECK(throwsRuntimeError([&]() { controller.use(feature); }));
    CHECK(!controller.hasFeature("test.consumer"));
}

TEST(EssentialFeature, CapabilityRequirementCanBeSatisfied) {
    EngineController engine{};
    EssentialsController controller{engine};
    test::TestFeature provider{};
    test::RequiresCapabilityFeature consumer{};

    controller.use(provider);
    CHECK(controller.hasCapability("test.capability"));

    controller.use(consumer);
    CHECK(controller.hasFeature("test.consumer"));
    CHECK(controller.hasCapability("test.consumer"));
}

TEST(EssentialFeature, CapabilityCanBeProvidedByAtomic) {
    EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};
    EssentialsController essentials{engine};
    test::RequiresAtomicExpressionFeature consumer{};

    CHECK(throwsRuntimeError([&]() { essentials.use(consumer); }));

    atomics.integer();
    CHECK(engine.hasCapability("expression.atom"));

    essentials.use(consumer);
    CHECK(essentials.hasFeature("test.atomic-consumer"));
}

TEST(EssentialFeature, FailedInstallationRollsBackControllerAndEngineState) {
    EngineController engine{};
    EssentialsController controller{engine};
    test::FailingTransactionalFeature feature{};

    CHECK(throwsRuntimeError([&]() { controller.use(feature); }));

    CHECK(controller.features().empty());
    CHECK(!controller.hasFeature("test.transaction.failure"));
    CHECK(!controller.hasFeature("test.essential"));
    CHECK(!controller.hasCapability("test.transaction.failure"));
    CHECK(!controller.hasCapability("test.capability"));
    CHECK(!engine.hasCapability("test.transaction.engine"));
    CHECK(!controller.functionRegistry().hasNative("transaction_native"));

    const auto tokens{engine.tokenize("transaction_keyword")};
    CHECK(tokens[0].kind == novac::token::Kind::Identifier);
}

TEST(EssentialFeature, FailedOwnedInstallationRollsBackState) {
    EngineController engine{};
    EssentialsController controller{engine};

    CHECK(throwsRuntimeError([&]() {
        controller.own(std::make_unique<test::FailingTransactionalFeature>());
    }));

    CHECK(controller.features().empty());
    CHECK(!controller.hasFeature("test.transaction.failure"));
    CHECK(!controller.hasFeature("test.essential"));
    CHECK(!controller.functionRegistry().hasNative("transaction_native"));

    // A clean installation after the rollback must still succeed.
    controller.use(test::TestFeature{});
    CHECK(controller.hasFeature("test.essential"));
}

TEST(EssentialPack, AddOwnsFeature) {
    EssentialPack pack{};
    pack.add<test::TestFeature>();

    CHECK(pack.features.size() == 1);
    CHECK(pack.features[0] != nullptr);
    CHECK(pack.features[0]->info().id == "test.essential");
}

TEST(EssentialPack, MergeMovesFeatures) {
    EssentialPack left{};
    EssentialPack right{};

    right.add<test::TestFeature>();
    left.merge(std::move(right));

    CHECK(left.features.size() == 1);
    CHECK(right.features.empty());
}

TEST(EssentialFeature, EmptyIdIsRejectedByController) {
    EngineController engine{};
    EssentialsController controller{engine};
    test::EmptyIdFeature feature{};

    CHECK(throwsRuntimeError([&]() {controller.use(feature);}));
}

TEST(EssentialFeature, DuplicateFeatureIsRejected) {
    EngineController engine{};
    EssentialsController controller{engine};
    test::TestFeature first{};
    test::TestFeature second{};

    controller.use(first);

    CHECK(throwsRuntimeError([&]() {controller.use(second);}));
}

} // namespace
