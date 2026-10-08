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
    /**
     * @brief Constructs a `MemoryController` instance.
     *
     * @param types Value supplied for `types`.
     * @param layouts Value supplied for `layouts`.
     * @param storage Value supplied for `storage`.
     */
    MemoryController(const types::TypeController &types, const types::LayoutController &layouts, const types::StorageController &storage);

    //Global behavior

    MemoryCapabilitySet &capabilities() noexcept;
    /**
     * @brief Returns the value exposed by `capabilities`.
     *
     * @return Value produced by the operation.
     */
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
    typename Operation::Result invoke(MemoryTarget target, const typename Operation::Request &request) {
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

    /**
     * @brief Invokes the operation handled by `invoke`.
     *
     * @param target Value supplied for `target`.
     * @param request Value supplied for `request`.
     * @return Value produced by the operation.
     */
    template <typename Operation>
    typename Operation::Result invoke(MemoryTarget target, const typename Operation::Request &request) const {
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

    //Address spaces

    AddressSpaceId registerAddressSpace(AddressSpaceDefinition definition, std::unique_ptr<BitAccess> access);

    /**
     * @brief Creates a value through `createAddressSpace`.
     *
     * @param name Value supplied for `name`.
     * @param bitSize Value supplied for `bitSize`.
     * @param access Value supplied for `access`.
     * @param extensions Value supplied for `extensions`.
     * @param capabilities Value supplied for `capabilities`.
     * @return Value produced by the operation.
     */
    AddressSpaceId createAddressSpace(std::string name, std::size_t bitSize, std::unique_ptr<BitAccess> access, types::ExtensionSet extensions = {}, MemoryCapabilitySet capabilities = {});

    /**
     * @brief Checks the condition represented by `hasAddressSpace`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    bool hasAddressSpace(AddressSpaceId id) const noexcept;
    /**
     * @brief Finds the value requested by `findAddressSpace`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    const AddressSpaceDefinition *findAddressSpace(AddressSpaceId id) const noexcept;
    /**
     * @brief Returns the value required by `requireAddressSpace`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    const AddressSpaceDefinition &requireAddressSpace(AddressSpaceId id) const;
    /**
     * @brief Returns the value exposed by `access`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    const BitAccess &access(AddressSpaceId id) const;
    /**
     * @brief Returns the value exposed by `access`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    BitAccess &access(AddressSpaceId id);

    //Regions

    RegionId createRegion(std::string name, AddressRange range, types::ExtensionSet extensions = {}, MemoryCapabilitySet capabilities = {});

    /**
     * @brief Sets the value handled by `setAllocationStrategy`.
     *
     * @param region Value supplied for `region`.
     * @param strategy Value supplied for `strategy`.
     */
    void setAllocationStrategy(RegionId region, std::unique_ptr<AllocationStrategy> strategy);

    /**
     * @brief Checks the condition represented by `hasRegion`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    bool hasRegion(RegionId id) const noexcept;
    /**
     * @brief Finds the value requested by `findRegion`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    const MemoryRegion *findRegion(RegionId id) const noexcept;
    /**
     * @brief Returns the value required by `requireRegion`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    const MemoryRegion &requireRegion(RegionId id) const;

    //Allocations

    Allocation allocateBits(RegionId region, std::size_t bitSize, std::size_t alignmentBits = 1, std::optional<LifetimeId> lifetime = std::nullopt, types::ExtensionSet extensions = {}, MemoryCapabilitySet capabilities = {});

    /**
     * @brief Allocates storage through `allocate`.
     *
     * @param region Value supplied for `region`.
     * @param type Value supplied for `type`.
     * @param lifetime Value supplied for `lifetime`.
     * @param extensions Value supplied for `extensions`.
     * @param capabilities Value supplied for `capabilities`.
     * @return Value produced by the operation.
     */
    MemoryReference allocate(RegionId region, types::TypeId type, std::optional<LifetimeId> lifetime = std::nullopt, types::ExtensionSet extensions = {}, MemoryCapabilitySet capabilities = {});

    /**
     * @brief Releases or removes data through `release`.
     *
     * @param id Value supplied for `id`.
     */
    void release(AllocationId id);

    /**
     * @brief Finds the value requested by `findAllocation`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    const Allocation *findAllocation(AllocationId id) const noexcept;
    /**
     * @brief Returns the value required by `requireAllocation`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    const Allocation &requireAllocation(AllocationId id) const;
    /**
     * @brief Returns the value exposed by `allocationActive`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    bool allocationActive(AllocationId id) const;

    //Lifetimes

    LifetimeId beginLifetime(types::ExtensionSet extensions = {});
    /**
     * @brief Completes the operation represented by `endLifetime`.
     *
     * @param id Value supplied for `id`.
     */
    void endLifetime(LifetimeId id);
    /**
     * @brief Performs the `lifetimeAlive` operation.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    bool lifetimeAlive(LifetimeId id) const;
    /**
     * @brief Finds the value requested by `findLifetime`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    const Lifetime *findLifetime(LifetimeId id) const noexcept;
    /**
     * @brief Returns the value required by `requireLifetime`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    const Lifetime &requireLifetime(LifetimeId id) const;

    //References

    MemoryReference reference(Address address, types::TypeId type, std::optional<AllocationId> provenance = std::nullopt) const;

    /**
     * @brief Performs the `dereference` operation.
     *
     * @param reference Value supplied for `reference`.
     * @return Value produced by the operation.
     */
    AddressRange dereference(const MemoryReference &reference) const;
    /**
     * @brief Validates data through `validateReference`.
     *
     * @param reference Value supplied for `reference`.
     */
    void validateReference(const MemoryReference &reference) const;

    //Raw bit access

    types::BitValue loadBits(Address address, std::size_t bitSize) const;
    /**
     * @brief Stores data through `storeBits`.
     *
     * @param address Value supplied for `address`.
     * @param value Value supplied for `value`.
     */
    void storeBits(Address address, const types::BitValue &value);

    //Typed access

    types::BitValue load(const MemoryReference &reference) const;
    /**
     * @brief Stores data through `store`.
     *
     * @param reference Value supplied for `reference`.
     * @param value Value supplied for `value`.
     */
    void store(const MemoryReference &reference, const types::BitValue &value);

private:
    friend class MemoryContext;

    struct AddressSpaceRecord {
        AddressSpaceDefinition definition{};
        std::unique_ptr<BitAccess> access{};
    };

    /**
     * @brief Resolves data through `resolveHandler`.
     *
     * @param target Value supplied for `target`.
     * @return Value produced by the operation.
     */
    template <typename Operation>
    MemoryOperationHandler<Operation> *resolveHandler(MemoryTarget target) {
        using Handler = MemoryOperationHandler<Operation>;
        if (const auto *allocationId{std::get_if<AllocationId>(&target)}) {
            Allocation &allocation{requireAllocationMutable(*allocationId)};
            if (!allocation.active())
                throw MemoryError{MemoryErrorCode::ReleasedAllocation, "MemoryController::invoke: target allocation has been released"};
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

    /**
     * @brief Resolves data through `resolveHandler`.
     *
     * @param target Value supplied for `target`.
     * @return Value produced by the operation.
     */
    template <typename Operation>
    const MemoryOperationHandler<Operation> *resolveHandler(MemoryTarget target) const {
        using Handler = MemoryOperationHandler<Operation>;
        if (const auto *allocationId{std::get_if<AllocationId>(&target)}) {
            const Allocation &allocation{requireAllocation(*allocationId)};
            if (!allocation.active())
                throw MemoryError{MemoryErrorCode::ReleasedAllocation, "MemoryController::invoke: target allocation has been released"};
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

    /**
     * @brief Performs the `checkedAdd` operation.
     *
     * @param left Value supplied for `left`.
     * @param right Value supplied for `right`.
     * @param owner Value supplied for `owner`.
     * @return Value produced by the operation.
     */
    static std::size_t checkedAdd(std::size_t left, std::size_t right, const char *owner);
    /**
     * @brief Validates data through `validateAlignment`.
     *
     * @param alignmentBits Value supplied for `alignmentBits`.
     * @param owner Value supplied for `owner`.
     */
    static void validateAlignment(std::size_t alignmentBits, const char *owner);
    /**
     * @brief Checks the condition represented by `aligned`.
     *
     * @param bitOffset Value supplied for `bitOffset`.
     * @param alignmentBits Value supplied for `alignmentBits`.
     * @return Value produced by the operation.
     */
    static bool aligned(std::size_t bitOffset, std::size_t alignmentBits) noexcept;
    /**
     * @brief Performs the `targetFor` operation.
     *
     * @param reference Value supplied for `reference`.
     * @return Value produced by the operation.
     */
    static MemoryTarget targetFor(const MemoryReference &reference) noexcept;

    /**
     * @brief Returns the value required by `requireAddressSpaceRecord`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    AddressSpaceRecord &requireAddressSpaceRecord(AddressSpaceId id);
    /**
     * @brief Returns the value required by `requireAddressSpaceRecord`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    const AddressSpaceRecord &requireAddressSpaceRecord(AddressSpaceId id) const;
    /**
     * @brief Returns the value required by `requireRegionMutable`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    MemoryRegion &requireRegionMutable(RegionId id);
    /**
     * @brief Returns the value required by `requireAllocationMutable`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    Allocation &requireAllocationMutable(AllocationId id);
    /**
     * @brief Validates data through `validateAddressRange`.
     *
     * @param range Value supplied for `range`.
     * @param owner Value supplied for `owner`.
     */
    void validateAddressRange(const AddressRange &range, const char *owner) const;
    /**
     * @brief Checks the condition represented by `activeAllocationsIn`.
     *
     * @param range Value supplied for `range`.
     * @return Value produced by the operation.
     */
    std::vector<Allocation> activeAllocationsIn(const AddressRange &range) const;
    /**
     * @brief Performs the `nextAllocationId` operation.
     *
     * @return Value produced by the operation.
     */
    AllocationId nextAllocationId();
    /**
     * @brief Performs the `nextAddressSpaceId` operation.
     *
     * @return Value produced by the operation.
     */
    AddressSpaceId nextAddressSpaceId();
    /**
     * @brief Performs the `nextRegionId` operation.
     *
     * @return Value produced by the operation.
     */
    RegionId nextRegionId();
    /**
     * @brief Performs the `nextLifetimeId` operation.
     *
     * @return Value produced by the operation.
     */
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
