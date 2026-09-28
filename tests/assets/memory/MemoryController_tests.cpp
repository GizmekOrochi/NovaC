#include "../../tester.hpp"

#include "novac/assets/memory/MemoryController.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>

namespace {

using namespace novac::assets::memory;
using namespace novac::assets::types;

bool throwsCode(const std::function<void()> &fn, MemoryErrorCode expected) {
    try {
        fn();
    } catch (const MemoryError &error) {
        return error.code() == expected;
    }
    return false;
}

struct Fixture {
    TypeController types{};
    LayoutController layouts{types};
    StorageController storage{types, layouts};
    MemoryController memory{types, layouts, storage};
};

class OverlayStrategy final : public AllocationStrategy {
public:
    std::optional<AllocationPlacement> place(const AllocationRequest &, std::span<const Allocation>) override {
        return AllocationPlacement{0};
    }
};

class InvertingAccess final : public BitAccess {
public:
    explicit InvertingAccess(std::size_t bits) : storage_{bits} {}

    std::size_t bitSize() const noexcept override { return storage_.bitSize(); }

    BitValue loadBits(std::size_t bitOffset, std::size_t bitSize) const override {
        BitValue physical{storage_.loadBits(BitAddress{bitOffset}, bitSize)};
        for (std::size_t index{0}; index < bitSize; ++index)
            physical.setBit(index, !physical.bit(index));
        return physical;
    }

    void storeBits(std::size_t bitOffset, const BitValue &value) override {
        BitValue physical{value};
        for (std::size_t index{0}; index < physical.bitSize(); ++index)
            physical.setBit(index, !physical.bit(index));
        storage_.storeBits(BitAddress{bitOffset}, physical);
    }

private:
    BitStorage storage_;
};



struct TeleportMemoryOperation {
    struct Request {
        Address source{};
        Address destination{};
        std::size_t bitSize{0};
    };
    using Result = void;
};

class TeleportMemoryHandler final : public MemoryOperationHandler<TeleportMemoryOperation> {
public:
    void execute(MemoryContext &context, const Request &request) const override {
        const BitValue value{static_cast<const MemoryContext &>(context).access(request.source.space)
            .loadBits(request.source.bitOffset, request.bitSize)};
        context.access(request.destination.space).storeBits(request.destination.bitOffset, value);
    }
};

struct ScopeNameOperation {
    struct Request {};
    using Result = std::size_t;
};

class ConstantScopeHandler final : public MemoryOperationHandler<ScopeNameOperation> {
public:
    explicit ConstantScopeHandler(std::size_t value) : value_{value} {}
    std::size_t execute(MemoryContext &, const Request &) const override { return value_; }
private:
    std::size_t value_{0};
};

class ConstantLoadHandler final : public MemoryOperationHandler<LoadBitsOperation> {
public:
    explicit ConstantLoadHandler(std::size_t value = 0b10101010) : value_{value} {}

    BitValue execute(MemoryContext &, const Request &request) const override {
        return BitValue::fromUnsigned(value_, request.bitSize);
    }

private:
    std::size_t value_{0};
};

class InvertingStoreHandler final : public MemoryOperationHandler<StoreBitsOperation> {
public:
    void execute(MemoryContext &context, const Request &request) const override {
        BitValue physical{request.value};
        for (std::size_t index{0}; index < physical.bitSize(); ++index)
            physical.setBit(index, !physical.bit(index));
        context.access(request.address.space).storeBits(request.address.bitOffset, physical);
    }
};

class InvertedThreeBitStorage final : public StorageCapability {
public:
    BitValue load(const StorageContext &context, const TypeDefinition &, const BitStorage &storage, BitAddress address) const override {
        BitValue physical{context.loadBits(storage, address, 3)};
        for (std::size_t index{0}; index < 3; ++index)
            physical.setBit(index, !physical.bit(index));
        return physical;
    }

    void store(const StorageContext &context, const TypeDefinition &, BitStorage &storage, BitAddress address, const BitValue &value) const override {
        BitValue physical{value};
        for (std::size_t index{0}; index < 3; ++index)
            physical.setBit(index, !physical.bit(index));
        context.storeBits(storage, address, physical);
    }
};

TEST(Memory, RawBitAccessIsBitPrecise) {
    Fixture f;
    const auto ram{f.memory.createAddressSpace("ram", 16, std::make_unique<BitStorageAccess>(16))};

    f.memory.storeBits(Address{ram, 3}, BitValue::fromUnsigned(0b10101, 5));
    const BitValue value{f.memory.loadBits(Address{ram, 3}, 5)};

    CHECK_EQ(value.bitSize(), static_cast<std::size_t>(5));
    CHECK_EQ(value.toUnsigned(), static_cast<std::uint64_t>(0b10101));
}

TEST(Memory, AddressSpacesRemainIndependent) {
    Fixture f;
    const auto first{f.memory.createAddressSpace("first", 8, std::make_unique<BitStorageAccess>(8))};
    const auto second{f.memory.createAddressSpace("second", 8, std::make_unique<BitStorageAccess>(8))};

    f.memory.storeBits(Address{first, 0}, BitValue::fromUnsigned(3, 2));
    f.memory.storeBits(Address{second, 0}, BitValue::fromUnsigned(1, 2));

    CHECK_EQ(f.memory.loadBits(Address{first, 0}, 2).toUnsigned(), static_cast<std::uint64_t>(3));
    CHECK_EQ(f.memory.loadBits(Address{second, 0}, 2).toUnsigned(), static_cast<std::uint64_t>(1));
}

TEST(Memory, AddressSpaceBackendMustMatchDeclaredWidth) {
    Fixture f;
    CHECK(throwsCode([&] {
        (void)f.memory.createAddressSpace("bad", 8, std::make_unique<BitStorageAccess>(7));
    }, MemoryErrorCode::BackendMismatch));
}

TEST(Memory, RegionMustFitInsideAddressSpace) {
    Fixture f;
    const auto ram{f.memory.createAddressSpace("ram", 8, std::make_unique<BitStorageAccess>(8))};
    CHECK(throwsCode([&] {
        (void)f.memory.createRegion("bad", AddressRange{Address{ram, 4}, 5});
    }, MemoryErrorCode::OutOfBounds));
}

TEST(Memory, TypedAllocationsPackExactTwoBitValues) {
    Fixture f;
    f.types.definePrimitive("CardinalDirection").bits(2).alignmentBits(1).commit();
    const auto ram{f.memory.createAddressSpace("ram", 8, std::make_unique<BitStorageAccess>(8))};
    const auto all{f.memory.createRegion("all", AddressRange{Address{ram, 0}, 8})};

    const auto a{f.memory.allocate(all, TypeId{"CardinalDirection"})};
    const auto b{f.memory.allocate(all, TypeId{"CardinalDirection"})};
    const auto c{f.memory.allocate(all, TypeId{"CardinalDirection"})};
    const auto d{f.memory.allocate(all, TypeId{"CardinalDirection"})};

    CHECK_EQ(a.address.bitOffset, static_cast<std::size_t>(0));
    CHECK_EQ(b.address.bitOffset, static_cast<std::size_t>(2));
    CHECK_EQ(c.address.bitOffset, static_cast<std::size_t>(4));
    CHECK_EQ(d.address.bitOffset, static_cast<std::size_t>(6));

    f.memory.store(a, BitValue::fromUnsigned(0, 2));
    f.memory.store(b, BitValue::fromUnsigned(1, 2));
    f.memory.store(c, BitValue::fromUnsigned(2, 2));
    f.memory.store(d, BitValue::fromUnsigned(3, 2));

    CHECK_EQ(f.memory.load(a).toUnsigned(), static_cast<std::uint64_t>(0));
    CHECK_EQ(f.memory.load(b).toUnsigned(), static_cast<std::uint64_t>(1));
    CHECK_EQ(f.memory.load(c).toUnsigned(), static_cast<std::uint64_t>(2));
    CHECK_EQ(f.memory.load(d).toUnsigned(), static_cast<std::uint64_t>(3));
    CHECK_EQ(f.memory.loadBits(Address{ram, 0}, 8).toUnsigned(), static_cast<std::uint64_t>(0b11100100));
}

TEST(Memory, AllocationUsesAbsoluteBitAlignment) {
    Fixture f;
    const auto ram{f.memory.createAddressSpace("ram", 32, std::make_unique<BitStorageAccess>(32))};
    const auto region{f.memory.createRegion("odd", AddressRange{Address{ram, 1}, 20})};

    const Allocation allocation{f.memory.allocateBits(region, 2, 3)};
    CHECK_EQ(allocation.range.begin.bitOffset, static_cast<std::size_t>(3));
    CHECK_EQ(allocation.alignmentBits, static_cast<std::size_t>(3));
}

TEST(Memory, ReleasedAllocationCanBeReusedByDefaultStrategy) {
    Fixture f;
    const auto ram{f.memory.createAddressSpace("ram", 8, std::make_unique<BitStorageAccess>(8))};
    const auto region{f.memory.createRegion("all", AddressRange{Address{ram, 0}, 8})};

    const Allocation first{f.memory.allocateBits(region, 4)};
    f.memory.release(first.id);
    const Allocation second{f.memory.allocateBits(region, 4)};

    CHECK_EQ(second.range.begin.bitOffset, static_cast<std::size_t>(0));
    CHECK(!f.memory.allocationActive(first.id));
}


TEST(Memory, DefaultStrategyAvoidsAllocationsFromOverlappingRegions) {
    Fixture f;
    const auto ram{f.memory.createAddressSpace("ram", 16, std::make_unique<BitStorageAccess>(16))};
    const auto left{f.memory.createRegion("left", AddressRange{Address{ram, 0}, 12})};
    const auto right{f.memory.createRegion("right", AddressRange{Address{ram, 4}, 12})};

    const Allocation first{f.memory.allocateBits(left, 8)};
    const Allocation second{f.memory.allocateBits(right, 4)};

    CHECK_EQ(first.range.begin.bitOffset, static_cast<std::size_t>(0));
    CHECK_EQ(second.range.begin.bitOffset, static_cast<std::size_t>(8));
}

TEST(Memory, CustomAllocationStrategyMayIntentionallyOverlay) {
    Fixture f;
    const auto ram{f.memory.createAddressSpace("ram", 8, std::make_unique<BitStorageAccess>(8))};
    const auto region{f.memory.createRegion("overlay", AddressRange{Address{ram, 0}, 8})};
    f.memory.setAllocationStrategy(region, std::make_unique<OverlayStrategy>());

    const Allocation first{f.memory.allocateBits(region, 4)};
    const Allocation second{f.memory.allocateBits(region, 4)};

    CHECK_EQ(first.range.begin.bitOffset, second.range.begin.bitOffset);
}

TEST(Memory, LifetimeInvalidatesManagedReferenceButNotRawBits) {
    Fixture f;
    f.types.definePrimitive("u3").bits(3).alignmentBits(1).commit();
    const auto ram{f.memory.createAddressSpace("ram", 8, std::make_unique<BitStorageAccess>(8))};
    const auto region{f.memory.createRegion("all", AddressRange{Address{ram, 0}, 8})};
    const auto lifetime{f.memory.beginLifetime()};
    const auto ref{f.memory.allocate(region, TypeId{"u3"}, lifetime)};

    f.memory.store(ref, BitValue::fromUnsigned(5, 3));
    f.memory.endLifetime(lifetime);

    CHECK(throwsCode([&] { (void)f.memory.load(ref); }, MemoryErrorCode::DeadLifetime));
    CHECK_EQ(f.memory.loadBits(ref.address, 3).toUnsigned(), static_cast<std::uint64_t>(5));
}

TEST(Memory, ReleasedProvenanceInvalidatesTypedReference) {
    Fixture f;
    f.types.definePrimitive("u2").bits(2).alignmentBits(1).commit();
    const auto ram{f.memory.createAddressSpace("ram", 8, std::make_unique<BitStorageAccess>(8))};
    const auto region{f.memory.createRegion("all", AddressRange{Address{ram, 0}, 8})};
    const auto ref{f.memory.allocate(region, TypeId{"u2"})};

    f.memory.release(*ref.provenance);
    CHECK(throwsCode([&] { (void)f.memory.load(ref); }, MemoryErrorCode::ReleasedAllocation));
}

TEST(Memory, ProvenancePreventsTypedRangeFromEscapingAllocation) {
    Fixture f;
    f.types.definePrimitive("u4").bits(4).alignmentBits(1).commit();
    const auto ram{f.memory.createAddressSpace("ram", 16, std::make_unique<BitStorageAccess>(16))};
    const auto region{f.memory.createRegion("all", AddressRange{Address{ram, 0}, 16})};
    const Allocation allocation{f.memory.allocateBits(region, 4)};

    CHECK(throwsCode([&] {
        (void)f.memory.reference(Address{ram, 1}, TypeId{"u4"}, allocation.id);
    }, MemoryErrorCode::InvalidReference));
}

TEST(Memory, DereferenceResolvesExactTypedBitRange) {
    Fixture f;
    f.types.definePrimitive("u5").bits(5).alignmentBits(1).commit();
    const auto ram{f.memory.createAddressSpace("ram", 16, std::make_unique<BitStorageAccess>(16))};
    const auto ref{f.memory.reference(Address{ram, 6}, TypeId{"u5"})};

    const AddressRange range{f.memory.dereference(ref)};
    CHECK_EQ(range.begin.bitOffset, static_cast<std::size_t>(6));
    CHECK_EQ(range.bitSize, static_cast<std::size_t>(5));
}

TEST(Memory, RawTypedReferenceDoesNotRequireAllocation) {
    Fixture f;
    f.types.definePrimitive("u4").bits(4).alignmentBits(1).commit();
    const auto ram{f.memory.createAddressSpace("ram", 16, std::make_unique<BitStorageAccess>(16))};
    const auto ref{f.memory.reference(Address{ram, 7}, TypeId{"u4"})};
    f.memory.store(ref, BitValue::fromUnsigned(0xA, 4));
    CHECK_EQ(f.memory.load(ref).toUnsigned(), static_cast<std::uint64_t>(0xA));
}

TEST(Memory, CustomBitAccessControlsPhysicalAddressSpaceBehavior) {
    Fixture f;
    const auto weird{f.memory.createAddressSpace("weird", 8, std::make_unique<InvertingAccess>(8))};

    f.memory.storeBits(Address{weird, 0}, BitValue::fromUnsigned(0b101, 3));
    CHECK_EQ(f.memory.loadBits(Address{weird, 0}, 3).toUnsigned(), static_cast<std::uint64_t>(0b101));
}

TEST(Memory, TypedAccessReusesExistingStorageCapability) {
    Fixture f;
    auto primitive{f.types.definePrimitive("encoded3")};
    primitive.bits(3).alignmentBits(1);
    primitive.capability<StorageCapability, InvertedThreeBitStorage>();
    primitive.commit();

    const auto ram{f.memory.createAddressSpace("ram", 8, std::make_unique<BitStorageAccess>(8))};
    const auto region{f.memory.createRegion("all", AddressRange{Address{ram, 0}, 8})};
    const auto ref{f.memory.allocate(region, TypeId{"encoded3"})};

    f.memory.store(ref, BitValue::fromUnsigned(0b001, 3));

    CHECK_EQ(f.memory.load(ref).toUnsigned(), static_cast<std::uint64_t>(0b001));
    CHECK_EQ(f.memory.loadBits(ref.address, 3).toUnsigned(), static_cast<std::uint64_t>(0b110));
}

TEST(Memory, AllocationFailsWhenRegionHasNoRemainingSpace) {
    Fixture f;
    const auto ram{f.memory.createAddressSpace("ram", 4, std::make_unique<BitStorageAccess>(4))};
    const auto region{f.memory.createRegion("all", AddressRange{Address{ram, 0}, 4})};
    (void)f.memory.allocateBits(region, 4);

    CHECK(throwsCode([&] { (void)f.memory.allocateBits(region, 1); }, MemoryErrorCode::OutOfMemory));
}


TEST(Memory, UnknownFutureOperationCanBeDefinedOutsideNovaCCore) {
    Fixture f;
    const auto ram{f.memory.createAddressSpace("ram", 16, std::make_unique<BitStorageAccess>(16))};
    f.memory.storeBits(Address{ram, 0}, BitValue::fromUnsigned(0b10101, 5));

    f.memory.capabilities().emplace<MemoryOperationHandler<TeleportMemoryOperation>, TeleportMemoryHandler>();
    f.memory.invoke<TeleportMemoryOperation>(
        ram,
        TeleportMemoryOperation::Request{Address{ram, 0}, Address{ram, 8}, 5}
    );

    CHECK_EQ(f.memory.loadBits(Address{ram, 8}, 5).toUnsigned(), static_cast<std::uint64_t>(0b10101));
}

TEST(Memory, CapabilityResolutionUsesMostSpecificScope) {
    Fixture f;
    MemoryCapabilitySet spaceCapabilities;
    spaceCapabilities.emplace<MemoryOperationHandler<ScopeNameOperation>, ConstantScopeHandler>(1);
    const auto ram{f.memory.createAddressSpace("ram", 16, std::make_unique<BitStorageAccess>(16), ExtensionSet{}, std::move(spaceCapabilities))};

    MemoryCapabilitySet regionCapabilities;
    regionCapabilities.emplace<MemoryOperationHandler<ScopeNameOperation>, ConstantScopeHandler>(2);
    const auto region{f.memory.createRegion("region", AddressRange{Address{ram, 0}, 16}, ExtensionSet{}, std::move(regionCapabilities))};

    MemoryCapabilitySet allocationCapabilities;
    allocationCapabilities.emplace<MemoryOperationHandler<ScopeNameOperation>, ConstantScopeHandler>(3);
    const auto allocation{f.memory.allocateBits(region, 4, 1, std::nullopt, ExtensionSet{}, std::move(allocationCapabilities))};

    CHECK_EQ(f.memory.invoke<ScopeNameOperation>(ram, {}), static_cast<std::size_t>(1));
    CHECK_EQ(f.memory.invoke<ScopeNameOperation>(region, {}), static_cast<std::size_t>(2));
    CHECK_EQ(f.memory.invoke<ScopeNameOperation>(allocation.id, {}), static_cast<std::size_t>(3));
}

TEST(Memory, AddressSpaceMayOverrideBuiltInLoadOperation) {
    Fixture f;
    MemoryCapabilitySet capabilities;
    capabilities.emplace<MemoryOperationHandler<LoadBitsOperation>, ConstantLoadHandler>();
    const auto space{f.memory.createAddressSpace("synthetic", 8, std::make_unique<BitStorageAccess>(8), ExtensionSet{}, std::move(capabilities))};

    CHECK_EQ(f.memory.loadBits(Address{space, 0}, 4).toUnsigned(), static_cast<std::uint64_t>(0b1010));
}

TEST(Memory, TypedLoadUsesRegionCapabilityThroughAllocationProvenance) {
    Fixture f;
    f.types.definePrimitive("u4").bits(4).alignmentBits(1).commit();
    const auto ram{f.memory.createAddressSpace("ram", 8, std::make_unique<BitStorageAccess>(8))};

    MemoryCapabilitySet regionCapabilities;
    regionCapabilities.emplace<MemoryOperationHandler<LoadBitsOperation>, ConstantLoadHandler>(0b0011);
    const auto region{f.memory.createRegion("synthetic", AddressRange{Address{ram, 0}, 8}, ExtensionSet{}, std::move(regionCapabilities))};
    const auto ref{f.memory.allocate(region, TypeId{"u4"})};

    CHECK_EQ(f.memory.load(ref).toUnsigned(), static_cast<std::uint64_t>(0b0011));
}

TEST(Memory, TypedStoreUsesAllocationCapabilityFromProvenance) {
    Fixture f;
    f.types.definePrimitive("u4").bits(4).alignmentBits(1).commit();
    const auto ram{f.memory.createAddressSpace("ram", 8, std::make_unique<BitStorageAccess>(8))};
    const auto region{f.memory.createRegion("all", AddressRange{Address{ram, 0}, 8})};

    MemoryCapabilitySet allocationCapabilities;
    allocationCapabilities.emplace<MemoryOperationHandler<StoreBitsOperation>, InvertingStoreHandler>();
    const auto ref{f.memory.allocate(region, TypeId{"u4"}, std::nullopt, ExtensionSet{}, std::move(allocationCapabilities))};

    f.memory.store(ref, BitValue::fromUnsigned(0b0011, 4));

    CHECK_EQ(f.memory.loadBits(ref.address, 4).toUnsigned(), static_cast<std::uint64_t>(0b1100));
}

TEST(Memory, MissingCustomOperationReportsUnsupportedOperation) {
    Fixture f;
    const auto ram{f.memory.createAddressSpace("ram", 8, std::make_unique<BitStorageAccess>(8))};
    CHECK(throwsCode([&] {
        (void)f.memory.invoke<ScopeNameOperation>(ram, {});
    }, MemoryErrorCode::UnsupportedOperation));
}

} // namespace
