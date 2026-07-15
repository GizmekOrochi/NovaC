#include "../../tester.hpp"

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
        return {
            "test.essential",
            "1.0.0",
            "Test essential feature",
            {"TestNode"},
            {"test.trait"},
            {"test.requirement"}
        };
    }

    void install(novac::assets::essentials::EssentialsController &) const override {
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
    CHECK(info.requirements.size() == 1);
    CHECK(info.requirements[0] == "test.requirement");
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

    CHECK(throwsRuntimeError([&]() {
        controller.use(feature);
    }));
}

TEST(EssentialFeature, DuplicateFeatureIsRejected) {
    EngineController engine{};
    EssentialsController controller{engine};
    test::TestFeature first{};
    test::TestFeature second{};

    controller.use(first);

    CHECK(throwsRuntimeError([&]() {
        controller.use(second);
    }));
}

} // namespace
