#pragma once

#include "novac/assets/memory/MemoryContext.hpp"
#include "novac/assets/memory/access/BitAccess.hpp"
#include "novac/assets/memory/allocation/AllocationStrategy.hpp"
#include "novac/assets/memory/behavior/MemoryOperations.hpp"
#include "novac/assets/types/LayoutController.hpp"
#include "novac/assets/types/StorageController.hpp"
#include "novac/assets/types/TypeController.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

namespace novac::assets::memory {

/** Bit-precise, policy-light, behavior-extensible memory orchestration. */
class MemoryController final {
public:
    MemoryController(
        const types::TypeController &types,
        const types::LayoutController &layouts,
        const types::StorageController &storage
    );

    // Global behavior ------------------------------------------------------

    MemoryCapabilitySet &capabilities() noexcept;
    const MemoryCapabilitySet &capabilities() const noexcept;

    /**
     * Invokes an operation against one memory scope.
     *
     * Resolution order is Allocation -> Region -> AddressSpace -> global.
     * Region targets use Region -> AddressSpace -> global; AddressSpace targets
     * use AddressSpace -> global. Operation types and handlers may be defined by
     * clients without modifying NovaC core.
     */
    template <typename Operation>
    typename Operation::Result invoke(
        MemoryTarget target,
        const typename Operation::Request &request
    ) {
        auto *handler{resolveHandler<Operation>(target)};
        if (!handler) {
            throw MemoryError{
                MemoryErrorCode::UnsupportedOperation,
                std::string{"MemoryController::invoke: no handler for operation "} + typeid(Operation).name()
            };
        }
        MemoryContext context{*this};
        return handler->execute(context, request);
    }

    template <typename Operation>
    typename Operation::Result invoke(
        MemoryTarget target,
        const typename Operation::Request &request
    ) const {
        const auto *handler{resolveHandler<Operation>(target)};
        if (!handler) {
            throw MemoryError{
                MemoryErrorCode::UnsupportedOperation,
                std::string{"MemoryController::invoke: no handler for operation "} + typeid(Operation).name()
            };
        }
        MemoryContext context{*this};
        return handler->execute(context, request);
    }

    // Address spaces -------------------------------------------------------

    AddressSpaceId registerAddressSpace(
        AddressSpaceDefinition definition,
        std::unique_ptr<BitAccess> access
    );

    AddressSpaceId createAddressSpace(
        std::string name,
        std::size_t bitSize,
        std::unique_ptr<BitAccess> access,
        types::ExtensionSet extensions = {},
        MemoryCapabilitySet capabilities = {}
    );

    bool hasAddressSpace(AddressSpaceId id) const noexcept;
    const AddressSpaceDefinition *findAddressSpace(AddressSpaceId id) const noexcept;
    const AddressSpaceDefinition &requireAddressSpace(AddressSpaceId id) const;
    const BitAccess &access(AddressSpaceId id) const;
    BitAccess &access(AddressSpaceId id);

    // Regions --------------------------------------------------------------

    RegionId createRegion(
        std::string name,
        AddressRange range,
        types::ExtensionSet extensions = {},
        MemoryCapabilitySet capabilities = {}
    );

    void setAllocationStrategy(RegionId region, std::unique_ptr<AllocationStrategy> strategy);

    bool hasRegion(RegionId id) const noexcept;
    const MemoryRegion *findRegion(RegionId id) const noexcept;
    const MemoryRegion &requireRegion(RegionId id) const;

    // Allocations ----------------------------------------------------------

    Allocation allocateBits(
        RegionId region,
        std::size_t bitSize,
        std::size_t alignmentBits = 1,
        std::optional<LifetimeId> lifetime = std::nullopt,
        types::ExtensionSet extensions = {},
        MemoryCapabilitySet capabilities = {}
    );

    MemoryReference allocate(
        RegionId region,
        types::TypeId type,
        std::optional<LifetimeId> lifetime = std::nullopt,
        types::ExtensionSet extensions = {},
        MemoryCapabilitySet capabilities = {}
    );

    void release(AllocationId id);

    const Allocation *findAllocation(AllocationId id) const noexcept;
    const Allocation &requireAllocation(AllocationId id) const;
    bool allocationActive(AllocationId id) const;

    // Lifetimes ------------------------------------------------------------

    LifetimeId beginLifetime(types::ExtensionSet extensions = {});
    void endLifetime(LifetimeId id);
    bool lifetimeAlive(LifetimeId id) const;
    const Lifetime *findLifetime(LifetimeId id) const noexcept;
    const Lifetime &requireLifetime(LifetimeId id) const;

    // References -----------------------------------------------------------

    MemoryReference reference(
        Address address,
        types::TypeId type,
        std::optional<AllocationId> provenance = std::nullopt
    ) const;

    AddressRange dereference(const MemoryReference &reference) const;
    void validateReference(const MemoryReference &reference) const;

    // Raw bit access -------------------------------------------------------

    types::BitValue loadBits(Address address, std::size_t bitSize) const;
    void storeBits(Address address, const types::BitValue &value);

    // Typed access ---------------------------------------------------------

    types::BitValue load(const MemoryReference &reference) const;
    void store(const MemoryReference &reference, const types::BitValue &value);

private:
    friend class MemoryContext;

    struct AddressSpaceRecord {
        AddressSpaceDefinition definition{};
        std::unique_ptr<BitAccess> access{};
    };

    template <typename Operation>
    MemoryOperationHandler<Operation> *resolveHandler(MemoryTarget target) {
        using Handler = MemoryOperationHandler<Operation>;
        if (const auto *allocationId{std::get_if<AllocationId>(&target)}) {
            Allocation &allocation{requireAllocationMutable(*allocationId)};
            if (auto *handler{allocation.capabilities.template get<Handler>()})
                return handler;
            MemoryRegion &region{requireRegionMutable(allocation.region)};
            if (auto *handler{region.capabilities.template get<Handler>()})
                return handler;
            AddressSpaceRecord &space{requireAddressSpaceRecord(region.range.begin.space)};
            if (auto *handler{space.definition.capabilities.template get<Handler>()})
                return handler;
        } else if (const auto *regionId{std::get_if<RegionId>(&target)}) {
            MemoryRegion &region{requireRegionMutable(*regionId)};
            if (auto *handler{region.capabilities.template get<Handler>()})
                return handler;
            AddressSpaceRecord &space{requireAddressSpaceRecord(region.range.begin.space)};
            if (auto *handler{space.definition.capabilities.template get<Handler>()})
                return handler;
        } else if (const auto *spaceId{std::get_if<AddressSpaceId>(&target)}) {
            AddressSpaceRecord &space{requireAddressSpaceRecord(*spaceId)};
            if (auto *handler{space.definition.capabilities.template get<Handler>()})
                return handler;
        }
        return globalCapabilities_.template get<Handler>();
    }

    template <typename Operation>
    const MemoryOperationHandler<Operation> *resolveHandler(MemoryTarget target) const {
        using Handler = MemoryOperationHandler<Operation>;
        if (const auto *allocationId{std::get_if<AllocationId>(&target)}) {
            const Allocation &allocation{requireAllocation(*allocationId)};
            if (const auto *handler{allocation.capabilities.template get<Handler>()})
                return handler;
            const MemoryRegion &region{requireRegion(allocation.region)};
            if (const auto *handler{region.capabilities.template get<Handler>()})
                return handler;
            const AddressSpaceRecord &space{requireAddressSpaceRecord(region.range.begin.space)};
            if (const auto *handler{space.definition.capabilities.template get<Handler>()})
                return handler;
        } else if (const auto *regionId{std::get_if<RegionId>(&target)}) {
            const MemoryRegion &region{requireRegion(*regionId)};
            if (const auto *handler{region.capabilities.template get<Handler>()})
                return handler;
            const AddressSpaceRecord &space{requireAddressSpaceRecord(region.range.begin.space)};
            if (const auto *handler{space.definition.capabilities.template get<Handler>()})
                return handler;
        } else if (const auto *spaceId{std::get_if<AddressSpaceId>(&target)}) {
            const AddressSpaceRecord &space{requireAddressSpaceRecord(*spaceId)};
            if (const auto *handler{space.definition.capabilities.template get<Handler>()})
                return handler;
        }
        return globalCapabilities_.template get<Handler>();
    }

    static std::size_t checkedAdd(std::size_t left, std::size_t right, const char *owner);
    static void validateAlignment(std::size_t alignmentBits, const char *owner);
    static bool aligned(std::size_t bitOffset, std::size_t alignmentBits) noexcept;

    AddressSpaceRecord &requireAddressSpaceRecord(AddressSpaceId id);
    const AddressSpaceRecord &requireAddressSpaceRecord(AddressSpaceId id) const;
    MemoryRegion &requireRegionMutable(RegionId id);
    Allocation &requireAllocationMutable(AllocationId id);
    void validateAddressRange(const AddressRange &range, const char *owner) const;
    std::vector<Allocation> activeAllocationsIn(const AddressRange &range) const;
    AllocationId nextAllocationId();
    AddressSpaceId nextAddressSpaceId();
    RegionId nextRegionId();
    LifetimeId nextLifetimeId();

    const types::TypeController *types_{nullptr};
    const types::LayoutController *layouts_{nullptr};
    const types::StorageController *storage_{nullptr};

    MemoryCapabilitySet globalCapabilities_{};
    std::unordered_map<AddressSpaceId, AddressSpaceRecord, AddressSpaceIdHash> addressSpaces_{};
    std::unordered_map<std::string, AddressSpaceId> addressSpaceNames_{};
    std::unordered_map<RegionId, MemoryRegion, RegionIdHash> regions_{};
    std::unordered_map<RegionId, std::unique_ptr<AllocationStrategy>, RegionIdHash> allocationStrategies_{};
    std::unordered_map<AllocationId, Allocation, AllocationIdHash> allocations_{};
    std::unordered_map<LifetimeId, Lifetime, LifetimeIdHash> lifetimes_{};

    std::size_t nextAddressSpaceValue_{1};
    std::size_t nextRegionValue_{1};
    std::size_t nextAllocationValue_{1};
    std::size_t nextLifetimeValue_{1};
};

} // namespace novac::assets::memory
