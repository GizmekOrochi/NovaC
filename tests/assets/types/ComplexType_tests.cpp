#include "../../tester.hpp"

#include "novac/assets/types/LayoutController.hpp"
#include "novac/assets/types/StorageController.hpp"
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

class FixedFiveBitLayout final : public LayoutCapability {
public:
    TypeLayout compute(const LayoutContext &, const TypeDefinition &) const override {
        return TypeLayout{5, 1, {}, {}};
    }
};

class TwoBitPackedLayout final : public LayoutCapability {
public:
    TypeLayout compute(const LayoutContext &, const TypeDefinition &) const override {
        return TypeLayout{2, 1, {}, {}};
    }
};

class ThreeBitPackedLayout final : public LayoutCapability {
public:
    TypeLayout compute(const LayoutContext &, const TypeDefinition &) const override {
        return TypeLayout{3, 1, {}, {}};
    }
};

class CountingLayout final : public LayoutCapability {
public:
    explicit CountingLayout(std::shared_ptr<std::size_t> calls) : calls_{std::move(calls)} {}

    TypeLayout compute(const LayoutContext &, const TypeDefinition &) const override {
        ++*calls_;
        return TypeLayout{16, 8, {}, {}};
    }

private:
    std::shared_ptr<std::size_t> calls_;
};

struct FieldTag {
    int value{0};
};

class TwoBitStorage final : public StorageCapability {
public:
    BitValue load(
        const StorageContext &context,
        const TypeDefinition &,
        const BitStorage &storage,
        BitAddress address
    ) const override {
        return context.loadBits(storage, address, 2);
    }

    void store(
        const StorageContext &context,
        const TypeDefinition &,
        BitStorage &storage,
        BitAddress address,
        const BitValue &value
    ) const override {
        if (value.bitSize() != 2)
            throw std::runtime_error("TwoBitStorage: expected a 2-bit value");
        context.storeBits(storage, address, value);
    }
};

class InvertedThreeBitStorage final : public StorageCapability {
public:
    BitValue load(
        const StorageContext &context,
        const TypeDefinition &,
        const BitStorage &storage,
        BitAddress address
    ) const override {
        const BitValue physical{context.loadBits(storage, address, 3)};
        BitValue decoded{3};
        for (std::size_t index{0}; index < 3; ++index)
            decoded.setBit(index, !physical.bit(index));
        return decoded;
    }

    void store(
        const StorageContext &context,
        const TypeDefinition &,
        BitStorage &storage,
        BitAddress address,
        const BitValue &value
    ) const override {
        if (value.bitSize() != 3)
            throw std::runtime_error("InvertedThreeBitStorage: expected a 3-bit value");
        BitValue encoded{3};
        for (std::size_t index{0}; index < 3; ++index)
            encoded.setBit(index, !value.bit(index));
        context.storeBits(storage, address, encoded);
    }
};


class ParityEightStorage final : public StorageCapability {
public:
    BitValue load(
        const StorageContext &context,
        const TypeDefinition &,
        const BitStorage &storage,
        BitAddress address
    ) const override {
        const BitValue physical{context.loadBits(storage, address, 8)};
        bool parity{false};
        for (std::size_t index{0}; index < 7; ++index)
            parity = parity != physical.bit(index);
        if (physical.bit(7) != parity)
            throw std::runtime_error("ParityEightStorage: parity mismatch");
        return physical;
    }

    void store(
        const StorageContext &context,
        const TypeDefinition &,
        BitStorage &storage,
        BitAddress address,
        const BitValue &value
    ) const override {
        if (value.bitSize() != 8)
            throw std::runtime_error("ParityEightStorage: expected an 8-bit value");
        BitValue encoded{value};
        bool parity{false};
        for (std::size_t index{0}; index < 7; ++index)
            parity = parity != encoded.bit(index);
        encoded.setBit(7, parity);
        context.storeBits(storage, address, encoded);
    }
};

class EscapingStorage final : public StorageCapability {
public:
    BitValue load(
        const StorageContext &context,
        const TypeDefinition &,
        const BitStorage &storage,
        BitAddress address
    ) const override {
        return context.loadBits(storage, address, 3);
    }

    void store(
        const StorageContext &context,
        const TypeDefinition &,
        BitStorage &storage,
        BitAddress address,
        const BitValue &
    ) const override {
        context.storeBits(storage, address, BitValue{3});
    }
};

class LayoutQueryingStorage final : public StorageCapability {
public:
    BitValue load(
        const StorageContext &context,
        const TypeDefinition &,
        const BitStorage &storage,
        BitAddress address
    ) const override {
        (void)context.layoutOf(TypeId{"counted"});
        (void)context.layoutOf(TypeId{"counted"});
        (void)context.layoutOf(TypeId{"counted"});
        return context.loadBits(storage, address, 2);
    }

    void store(
        const StorageContext &context,
        const TypeDefinition &,
        BitStorage &storage,
        BitAddress address,
        const BitValue &value
    ) const override {
        (void)context.layoutOf(TypeId{"counted"});
        (void)context.layoutOf(TypeId{"counted"});
        (void)context.layoutOf(TypeId{"counted"});
        context.storeBits(storage, address, value);
    }
};

TEST(ComplexTypes, StructUsesGenericCapabilities) {
    TypeController types;
    types.definePrimitive("i32").bits(32).alignmentBytes(4).commit();

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

TEST(ComplexTypes, NaturalStructLayoutComputesPaddingInBits) {
    TypeController types;
    types.definePrimitive("short").bits(16).alignmentBytes(2).commit();
    types.definePrimitive("int").bits(32).alignmentBytes(4).commit();

    defineStruct(types, "Example")
        .field("a", TypeId{"short"})
        .field("b", TypeId{"int"})
        .field("c", TypeId{"short"})
        .commit();

    LayoutController layouts{types};
    const TypeLayout layout{layouts.compute(TypeId{"Example"})};

    CHECK_EQ(layout.bitSize, static_cast<std::size_t>(96));
    CHECK_EQ(layout.alignmentBits, static_cast<std::size_t>(32));
    CHECK_EQ(layout.sizeBytes(), static_cast<std::size_t>(12));
    CHECK_EQ(layout.alignmentBytes(), static_cast<std::size_t>(4));
    CHECK_EQ(layout.components.size(), static_cast<std::size_t>(3));
    CHECK_EQ(layout.components[0].bitOffset, static_cast<std::size_t>(0));
    CHECK_EQ(layout.components[1].bitOffset, static_cast<std::size_t>(32));
    CHECK_EQ(layout.components[2].bitOffset, static_cast<std::size_t>(64));
    CHECK_EQ(layout.components[1].byteOffset(), static_cast<std::size_t>(4));
}

TEST(ComplexTypes, MemberLookupKeepsComponentMetadata) {
    TypeController types;
    types.definePrimitive("f32").bits(32).alignmentBytes(4).commit();
    defineStruct(types, "Vec2")
        .field("x", TypeId{"f32"})
        .field("y", TypeId{"f32"})
        .fieldExtension<FieldTag>("y", FieldTag{42})
        .commit();

    const TypeDefinition &vec{types.requireType(TypeId{"Vec2"})};
    const auto *members{vec.capabilities.get<MemberCapability>()};
    CHECK(members != nullptr);

    const auto y{members->findMember(vec, "y")};
    CHECK(y.has_value());
    CHECK(y->type() == TypeId{"f32"});
    CHECK(y->name() == "y");
    CHECK_EQ(y->componentIndex, static_cast<std::size_t>(1));
    CHECK(y->extensions().has<FieldTag>());
    CHECK_EQ(y->extensions().get<FieldTag>()->value, 42);
    CHECK(!members->findMember(vec, "z").has_value());
}

TEST(ComplexTypes, CustomTypeCanProvideCompletelyCustomLayoutBehavior) {
    TypeController types;
    auto weird{std::make_unique<WeirdType>(TypeId{"weird"})};
    weird->capabilities.emplace<LayoutCapability, WeirdLayout>();
    types.registerType(std::move(weird));

    LayoutController layouts{types};
    const TypeLayout layout{layouts.compute(TypeId{"weird"})};
    CHECK_EQ(layout.bitSize, static_cast<std::size_t>(13));
    CHECK_EQ(layout.alignmentBits, static_cast<std::size_t>(3));
    CHECK_EQ(layout.sizeBytes(), static_cast<std::size_t>(2));
}

TEST(ComplexTypes, PrimitiveBitAlignmentPacksSubByteStorageWithoutCustomLayout) {
    TypeController types;
    types.definePrimitive("direction")
        .bits(2)
        .alignmentBits(1)
        .unsignedType()
        .commit();

    defineStruct(types, "Directions")
        .field("left", TypeId{"direction"})
        .field("up", TypeId{"direction"})
        .field("right", TypeId{"direction"})
        .field("down", TypeId{"direction"})
        .commit();

    LayoutController layouts{types};
    const TypeLayout layout{layouts.compute(TypeId{"Directions"})};
    CHECK_EQ(layout.bitSize, static_cast<std::size_t>(8));
    CHECK_EQ(layout.alignmentBits, static_cast<std::size_t>(1));
    CHECK_EQ(layout.components[0].bitOffset, static_cast<std::size_t>(0));
    CHECK_EQ(layout.components[1].bitOffset, static_cast<std::size_t>(2));
    CHECK_EQ(layout.components[2].bitOffset, static_cast<std::size_t>(4));
    CHECK_EQ(layout.components[3].bitOffset, static_cast<std::size_t>(6));
}

TEST(ComplexTypes, PrimitiveCustomLayoutCannotChangeDeclaredBitSize) {
    TypeController types;
    types.definePrimitive("two")
        .bits(2)
        .alignmentBits(1)
        .capability<LayoutCapability, ThreeBitPackedLayout>()
        .commit();

    LayoutController layouts{types};
    CHECK(throws([&] {
        (void)layouts.compute(TypeId{"two"});
    }));
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

TEST(ComplexTypes, MutuallyRecursiveByValueLayoutsAreRejected) {
    TypeController types;
    types.declareType("A");
    types.declareType("B");

    defineStruct(types, "A").field("b", TypeId{"B"}).commit();
    defineStruct(types, "B").field("a", TypeId{"A"}).commit();

    LayoutController layouts{types};
    CHECK(throws([&] { (void)layouts.compute(TypeId{"A"}); }));
}

TEST(ComplexTypes, LayoutCanNestStructsRecursivelyWhenAcyclic) {
    TypeController types;
    types.definePrimitive("i32").bits(32).alignmentBytes(4).commit();
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
    CHECK_EQ(line.bitSize, static_cast<std::size_t>(128));
    CHECK_EQ(line.alignmentBits, static_cast<std::size_t>(32));
    CHECK_EQ(line.components[1].bitOffset, static_cast<std::size_t>(64));
}

TEST(ComplexTypes, StructFieldsCanonicalizeAliases) {
    TypeController types;
    types.definePrimitive("i32").bits(32).alignmentBytes(4).commit();
    types.registerAlias("word", TypeId{"i32"});

    defineStruct(types, "Box").field("value", TypeId{"word"}).commit();

    const auto &box{dynamic_cast<const StructType &>(types.requireType(TypeId{"Box"}))};
    CHECK(box.fields()[0].type == TypeId{"i32"});
}

TEST(ComplexTypes, NaturalStructLayoutConsumesCustomNestedLayouts) {
    TypeController types;
    types.definePrimitive("u8").bits(8).alignmentBytes(1).commit();

    auto weird{std::make_unique<WeirdType>(TypeId{"weird"})};
    weird->capabilities.emplace<LayoutCapability, WeirdLayout>();
    types.registerType(std::move(weird));

    defineStruct(types, "Container")
        .field("tag", TypeId{"u8"})
        .field("payload", TypeId{"weird"})
        .commit();

    LayoutController layouts{types};
    const TypeLayout layout{layouts.compute(TypeId{"Container"})};
    CHECK_EQ(layout.components[0].bitOffset, static_cast<std::size_t>(0));
    CHECK_EQ(layout.components[1].bitOffset, static_cast<std::size_t>(9));
    CHECK_EQ(layout.bitSize, static_cast<std::size_t>(24));
    CHECK_EQ(layout.alignmentBits, static_cast<std::size_t>(8));
}

TEST(ComplexTypes, StructBuilderCanReplaceDefaultLayoutCapability) {
    TypeController types;
    types.definePrimitive("i32").bits(32).alignmentBytes(4).commit();

    defineStruct(types, "Compressed")
        .field("value", TypeId{"i32"})
        .capability<LayoutCapability, FixedFiveBitLayout>()
        .commit();

    LayoutController layouts{types};
    const TypeLayout layout{layouts.compute(TypeId{"Compressed"})};
    CHECK_EQ(layout.bitSize, static_cast<std::size_t>(5));
    CHECK_EQ(layout.alignmentBits, static_cast<std::size_t>(1));
}

TEST(ComplexTypes, LayoutResolutionCachesRepeatedNestedTypesPerComputation) {
    TypeController types;
    const auto calls{std::make_shared<std::size_t>(0)};
    auto counted{std::make_unique<WeirdType>(TypeId{"counted"})};
    counted->capabilities.emplace<LayoutCapability, CountingLayout>(calls);
    types.registerType(std::move(counted));

    defineStruct(types, "Repeated")
        .field("a", TypeId{"counted"})
        .field("b", TypeId{"counted"})
        .field("c", TypeId{"counted"})
        .commit();

    LayoutController layouts{types};
    (void)layouts.compute(TypeId{"Repeated"});
    CHECK_EQ(*calls, static_cast<std::size_t>(1));
}

TEST(ComplexTypes, BitStoragePacksFourTwoBitValuesIntoOneByte) {
    BitStorage storage{8};
    storage.storeBits(BitAddress{0}, BitValue::fromUnsigned(0, 2));
    storage.storeBits(BitAddress{2}, BitValue::fromUnsigned(1, 2));
    storage.storeBits(BitAddress{4}, BitValue::fromUnsigned(2, 2));
    storage.storeBits(BitAddress{6}, BitValue::fromUnsigned(3, 2));

    CHECK_EQ(storage.byteSize(), static_cast<std::size_t>(1));
    CHECK_EQ(storage.bytes()[0], static_cast<std::uint8_t>(0xE4));
    CHECK_EQ(storage.loadBits(BitAddress{0}, 2).toUnsigned(), std::uint64_t{0});
    CHECK_EQ(storage.loadBits(BitAddress{2}, 2).toUnsigned(), std::uint64_t{1});
    CHECK_EQ(storage.loadBits(BitAddress{4}, 2).toUnsigned(), std::uint64_t{2});
    CHECK_EQ(storage.loadBits(BitAddress{6}, 2).toUnsigned(), std::uint64_t{3});
}

TEST(ComplexTypes, StorageControllerUsesTypeStorageCapability) {
    TypeController types;
    types.definePrimitive("direction")
        .bits(2)
        .alignmentBits(1)
        .unsignedType()
        .capability<StorageCapability, TwoBitStorage>()
        .commit();

    LayoutController layouts{types};
    StorageController storageController{types, layouts};
    BitStorage storage{8};

    storageController.store(TypeId{"direction"}, storage, BitAddress{4}, BitValue::fromUnsigned(3, 2));
    CHECK_EQ(storageController.load(TypeId{"direction"}, storage, BitAddress{4}).toUnsigned(), std::uint64_t{3});
    CHECK(storage.bit(4));
    CHECK(storage.bit(5));
    CHECK(!storage.bit(3));
    CHECK(!storage.bit(6));
}

TEST(ComplexTypes, CustomStorageBehaviorCanEncodePhysicalBitsDifferently) {
    TypeController types;
    types.definePrimitive("inverted3")
        .bits(3)
        .alignmentBits(1)
        .unsignedType()
        .capability<StorageCapability, InvertedThreeBitStorage>()
        .commit();

    LayoutController layouts{types};
    StorageController storageController{types, layouts};
    BitStorage storage{16};

    storageController.store(TypeId{"inverted3"}, storage, BitAddress{1}, BitValue::fromUnsigned(1, 3));
    CHECK_EQ(storage.loadBits(BitAddress{1}, 3).toUnsigned(), std::uint64_t{6});
    CHECK_EQ(storageController.load(TypeId{"inverted3"}, storage, BitAddress{1}).toUnsigned(), std::uint64_t{1});
}

TEST(ComplexTypes, BitValueSupportsLowLevelExtractInsertShiftAndMask) {
    BitValue value{BitValue::fromUnsigned(0b101101, 6)};
    CHECK_EQ(value.extract(1, 3).toUnsigned(), std::uint64_t{0b110});
    CHECK_EQ(value.shiftedLeft(1).toUnsigned(), std::uint64_t{0b011010});
    CHECK_EQ(value.shiftedRight(2).toUnsigned(), std::uint64_t{0b001011});
    CHECK_EQ(value.masked(BitValue::fromUnsigned(0b001111, 6)).toUnsigned(), std::uint64_t{0b001101});

    BitValue destination{8};
    destination.insert(3, BitValue::fromUnsigned(0b101, 3));
    CHECK_EQ(destination.toUnsigned(), std::uint64_t{0b101000});
}


TEST(ComplexTypes, StorageCapabilityCanValidateAndNormalizeTheTypesRealBits) {
    TypeController types;
    types.definePrimitive("parity8")
        .bits(8)
        .alignmentBits(8)
        .unsignedType()
        .capability<StorageCapability, ParityEightStorage>()
        .commit();

    LayoutController layouts{types};
    StorageController storageController{types, layouts};
    BitStorage storage{8};

    const BitValue value{BitValue::fromUnsigned(0b0010110, 8)};
    storageController.store(TypeId{"parity8"}, storage, BitAddress{0}, value);

    const BitValue loaded{storageController.load(TypeId{"parity8"}, storage, BitAddress{0})};
    CHECK_EQ(loaded.bitSize(), static_cast<std::size_t>(8));
    CHECK_EQ(layouts.compute(TypeId{"parity8"}).bitSize, static_cast<std::size_t>(8));
    CHECK_EQ(loaded.extract(0, 7).toUnsigned(), value.extract(0, 7).toUnsigned());
}

TEST(ComplexTypes, StorageCapabilityCannotEscapeItsPhysicalRange) {
    TypeController types;
    types.definePrimitive("bounded2")
        .bits(2)
        .alignmentBits(1)
        .unsignedType()
        .capability<StorageCapability, EscapingStorage>()
        .commit();

    LayoutController layouts{types};
    StorageController storageController{types, layouts};
    BitStorage storage{16};

    CHECK(throws([&] {
        storageController.store(TypeId{"bounded2"}, storage, BitAddress{4}, BitValue::fromUnsigned(1, 2));
    }));
    CHECK(throws([&] {
        (void)storageController.load(TypeId{"bounded2"}, storage, BitAddress{4});
    }));
}

TEST(ComplexTypes, StorageOperationSharesOneLayoutResolutionSession) {
    TypeController types;
    const auto calls{std::make_shared<std::size_t>(0)};

    auto counted{std::make_unique<WeirdType>(TypeId{"counted"})};
    counted->capabilities.emplace<LayoutCapability, CountingLayout>(calls);
    types.registerType(std::move(counted));

    types.definePrimitive("querying2")
        .bits(2)
        .alignmentBits(1)
        .unsignedType()
        .capability<StorageCapability, LayoutQueryingStorage>()
        .commit();

    LayoutController layouts{types};
    StorageController storageController{types, layouts};
    BitStorage storage{8};

    storageController.store(TypeId{"querying2"}, storage, BitAddress{0}, BitValue::fromUnsigned(2, 2));
    CHECK_EQ(*calls, static_cast<std::size_t>(1));

    *calls = 0;
    CHECK_EQ(storageController.load(TypeId{"querying2"}, storage, BitAddress{0}).toUnsigned(), std::uint64_t{2});
    CHECK_EQ(*calls, static_cast<std::size_t>(1));
}

} // namespace
