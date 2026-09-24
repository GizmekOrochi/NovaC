#include "../../tester.hpp"

#include "novac/assets/types/LayoutController.hpp"
#include "novac/assets/types/aggregate/StructType.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

using namespace novac::assets::types;
using namespace novac::assets::types::aggregate;

bool throws(const std::function<void()> &fn) {
    try { fn(); } catch (const std::exception &) { return true; }
    return false;
}

class WeirdType final : public TypeDefinition {
public:
    explicit WeirdType(TypeId id) : TypeDefinition{std::move(id)} {}
};

class WeirdLayout final : public LayoutCapability {
public:
    TypeLayout compute(const LayoutContext &, const TypeDefinition &) const override {
        return TypeLayout{13, 3, {}, {}};
    }
};

TEST(ComplexTypes, StructUsesGenericCapabilities) {
    TypeController types;
    types.definePrimitive("i32").bits(32).storageBits(32).alignment(4).commit();

    CHECK(defineStruct(types, "Point")
        .field("x", TypeId{"i32"})
        .field("y", TypeId{"i32"})
        .commit() == novac::registry::RegisterStatus::Inserted);

    const auto &point = dynamic_cast<const StructType &>(types.requireType(TypeId{"Point"}));
    CHECK(point.capabilities.has<CompositionCapability>());
    CHECK(point.capabilities.has<LayoutCapability>());
    CHECK(point.capabilities.has<MemberCapability>());
    CHECK_EQ(point.fields().size(), static_cast<std::size_t>(2));
}

TEST(ComplexTypes, NaturalStructLayoutComputesPadding) {
    TypeController types;
    types.definePrimitive("short").bits(16).storageBits(16).alignment(2).commit();
    types.definePrimitive("int").bits(32).storageBits(32).alignment(4).commit();

    defineStruct(types, "Example")
        .field("a", TypeId{"short"})
        .field("b", TypeId{"int"})
        .field("c", TypeId{"short"})
        .commit();

    LayoutController layouts{types};
    const TypeLayout layout{layouts.compute(TypeId{"Example"})};

    CHECK_EQ(layout.size, static_cast<std::size_t>(12));
    CHECK_EQ(layout.alignment, static_cast<std::size_t>(4));
    CHECK_EQ(layout.components.size(), static_cast<std::size_t>(3));
    CHECK_EQ(layout.components[0].offset, static_cast<std::size_t>(0));
    CHECK_EQ(layout.components[1].offset, static_cast<std::size_t>(4));
    CHECK_EQ(layout.components[2].offset, static_cast<std::size_t>(8));
}

TEST(ComplexTypes, MemberLookupIsBehaviorNotHardcodedStructLogic) {
    TypeController types;
    types.definePrimitive("f32").bits(32).storageBits(32).alignment(4).commit();
    defineStruct(types, "Vec2")
        .field("x", TypeId{"f32"})
        .field("y", TypeId{"f32"})
        .commit();

    const TypeDefinition &vec{types.requireType(TypeId{"Vec2"})};
    const auto *members{vec.capabilities.get<MemberCapability>()};
    CHECK(members != nullptr);

    const auto y{members->findMember(vec, "y")};
    CHECK(y.has_value());
    CHECK(y->type == TypeId{"f32"});
    CHECK_EQ(y->componentIndex, static_cast<std::size_t>(1));
    CHECK(!members->findMember(vec, "z").has_value());
}

TEST(ComplexTypes, CustomTypeCanProvideCompletelyCustomLayoutBehavior) {
    TypeController types;
    auto weird{std::make_unique<WeirdType>(TypeId{"weird"})};
    weird->capabilities.emplace<LayoutCapability, WeirdLayout>();
    types.registerType(std::move(weird));

    LayoutController layouts{types};
    const TypeLayout layout{layouts.compute(TypeId{"weird"})};
    CHECK_EQ(layout.size, static_cast<std::size_t>(13));
    CHECK_EQ(layout.alignment, static_cast<std::size_t>(3));
}

TEST(ComplexTypes, CapabilitiesFreezeWithRegisteredType) {
    TypeController types;
    auto weird{std::make_unique<WeirdType>(TypeId{"weird"})};
    weird->capabilities.emplace<LayoutCapability, WeirdLayout>();
    types.registerType(std::move(weird));

    const TypeDefinition &stored{types.requireType(TypeId{"weird"})};
    CHECK(stored.capabilities.frozen());
    CHECK(throws([&] {
        const_cast<TypeCapabilities &>(stored.capabilities).erase<LayoutCapability>();
    }));
}

TEST(ComplexTypes, StructRejectsDuplicateAndUnknownFields) {
    TypeController types;
    types.definePrimitive("i32").bits(32).commit();

    CHECK(throws([&] {
        auto builder{defineStruct(types, "Bad")};
        builder.field("x", TypeId{"i32"}).field("x", TypeId{"i32"});
    }));

    CHECK(throws([&] {
        defineStruct(types, "Unknown")
            .field("value", TypeId{"Missing"})
            .commit();
    }));
}

TEST(ComplexTypes, RecursiveByValueLayoutsAreRejected) {
    TypeController types;
    defineStruct(types, "Node")
        .field("next", TypeId{"Node"})
        .commit();

    LayoutController layouts{types};
    CHECK(throws([&] { (void)layouts.compute(TypeId{"Node"}); }));
}

TEST(ComplexTypes, LayoutCanNestStructsRecursivelyWhenAcyclic) {
    TypeController types;
    types.definePrimitive("i32").bits(32).storageBits(32).alignment(4).commit();
    defineStruct(types, "Point")
        .field("x", TypeId{"i32"})
        .field("y", TypeId{"i32"})
        .commit();
    defineStruct(types, "Line")
        .field("a", TypeId{"Point"})
        .field("b", TypeId{"Point"})
        .commit();

    LayoutController layouts{types};
    const TypeLayout line{layouts.compute(TypeId{"Line"})};
    CHECK_EQ(line.size, static_cast<std::size_t>(16));
    CHECK_EQ(line.alignment, static_cast<std::size_t>(4));
    CHECK_EQ(line.components[1].offset, static_cast<std::size_t>(8));
}

} // namespace
