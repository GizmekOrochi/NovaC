#include "novac/assets/memory/MemoryController.hpp"

#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace novac::assets::memory {

/**
 * @brief Constructs a `MemoryController` instance.
 *
 * @param types Value supplied for `types`.
 * @param layouts Value supplied for `layouts`.
 * @param storage Value supplied for `storage`.
 */
MemoryController::MemoryController(const types::TypeController &types, const types::LayoutController &layouts, const types::StorageController &storage)
     : types_{&types}, layouts_{&layouts}, storage_{&storage} {}

/**
 * @brief Returns the value exposed by `capabilities`.
 *
 * @return Value produced by the operation.
 */
MemoryCapabilitySet &MemoryController::capabilities() noexcept { return globalCapabilities_; }
/**
 * @brief Returns the value exposed by `capabilities`.
 *
 * @return Value produced by the operation.
 */
const MemoryCapabilitySet &MemoryController::capabilities() const noexcept { return globalCapabilities_; }

namespace {

/**
 * @brief Implements the `idText` operation.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
std::string idText(std::size_t value) {
    return std::to_string(value);
}

} // namespace

/**
 * @brief Implements the `checkedAdd` operation.
 *
 * @param left Value supplied for `left`.
 * @param right Value supplied for `right`.
 * @param owner Value supplied for `owner`.
 * @return Value produced by the operation.
 */
std::size_t MemoryController::checkedAdd(std::size_t left, std::size_t right, const char *owner) {
    if (left > std::numeric_limits<std::size_t>::max() - right)
        throw MemoryError{MemoryErrorCode::OutOfBounds, std::string{owner} + ": bit offset overflow"};
    return left + right;
}

/**
 * @brief Validates data through `validateAlignment`.
 *
 * @param alignmentBits Value supplied for `alignmentBits`.
 * @param owner Value supplied for `owner`.
 */
void MemoryController::validateAlignment(std::size_t alignmentBits, const char *owner) {
    if (alignmentBits == 0)
        throw MemoryError{MemoryErrorCode::InvalidAlignment, std::string{owner} + ": alignment must be at least one bit"};
}

/**
 * @brief Checks the condition represented by `aligned`.
 *
 * @param bitOffset Value supplied for `bitOffset`.
 * @param alignmentBits Value supplied for `alignmentBits`.
 * @return Value produced by the operation.
 */
bool MemoryController::aligned(std::size_t bitOffset, std::size_t alignmentBits) noexcept {
    return alignmentBits != 0 && bitOffset % alignmentBits == 0;
}

/**
 * @brief Implements the `nextAddressSpaceId` operation.
 *
 * @return Value produced by the operation.
 */
AddressSpaceId MemoryController::nextAddressSpaceId() {
    if (nextAddressSpaceValue_ == 0)
        throw MemoryError{MemoryErrorCode::InvalidArgument, "MemoryController: address-space id space exhausted"};
    const AddressSpaceId id{nextAddressSpaceValue_++};
    if (nextAddressSpaceValue_ == 0 && id.value != std::numeric_limits<std::size_t>::max())
        throw MemoryError{MemoryErrorCode::InvalidArgument, "MemoryController: address-space id overflow"};
    return id;
}

/**
 * @brief Implements the `nextRegionId` operation.
 *
 * @return Value produced by the operation.
 */
RegionId MemoryController::nextRegionId() {
    if (nextRegionValue_ == 0)
        throw MemoryError{MemoryErrorCode::InvalidArgument, "MemoryController: region id space exhausted"};
    return RegionId{nextRegionValue_++};
}

/**
 * @brief Implements the `nextAllocationId` operation.
 *
 * @return Value produced by the operation.
 */
AllocationId MemoryController::nextAllocationId() {
    if (nextAllocationValue_ == 0)
        throw MemoryError{MemoryErrorCode::InvalidArgument, "MemoryController: allocation id space exhausted"};
    return AllocationId{nextAllocationValue_++};
}

/**
 * @brief Implements the `nextLifetimeId` operation.
 *
 * @return Value produced by the operation.
 */
LifetimeId MemoryController::nextLifetimeId() {
    if (nextLifetimeValue_ == 0)
        throw MemoryError{MemoryErrorCode::InvalidArgument, "MemoryController: lifetime id space exhausted"};
    return LifetimeId{nextLifetimeValue_++};
}

/**
 * @brief Registers data through `registerAddressSpace`.
 *
 * @param definition Value supplied for `definition`.
 * @param accessBackend Value supplied for `accessBackend`.
 * @return Value produced by the operation.
 */
AddressSpaceId MemoryController::registerAddressSpace(AddressSpaceDefinition definition, std::unique_ptr<BitAccess> accessBackend) {
    if (definition.id.valid())
        throw MemoryError{MemoryErrorCode::InvalidArgument, "MemoryController::registerAddressSpace: ids are assigned by the controller"};
    if (definition.name.empty())
        throw MemoryError{MemoryErrorCode::InvalidArgument, "MemoryController::registerAddressSpace: name cannot be empty"};
    if (definition.bitSize == 0)
        throw MemoryError{MemoryErrorCode::InvalidArgument, "MemoryController::registerAddressSpace: size must be greater than zero"};
    if (!accessBackend)
        throw MemoryError{MemoryErrorCode::InvalidArgument, "MemoryController::registerAddressSpace: BitAccess backend cannot be null"};
    if (accessBackend->bitSize() != definition.bitSize) {
        throw MemoryError{
            MemoryErrorCode::BackendMismatch,
            "MemoryController::registerAddressSpace: backend exposes " + std::to_string(accessBackend->bitSize()) +
                " bits but address space declares " + std::to_string(definition.bitSize)
        };
    }
    if (addressSpaceNames_.find(definition.name) != addressSpaceNames_.end())
        throw MemoryError{MemoryErrorCode::DuplicateName, "MemoryController::registerAddressSpace: duplicate address-space name '" + definition.name + "'"};

    definition.id = nextAddressSpaceId();
    if (!definition.capabilities.has<MemoryOperationHandler<LoadBitsOperation>>())
        definition.capabilities.emplace<MemoryOperationHandler<LoadBitsOperation>, DirectLoadBitsHandler>();
    if (!definition.capabilities.has<MemoryOperationHandler<StoreBitsOperation>>())
        definition.capabilities.emplace<MemoryOperationHandler<StoreBitsOperation>, DirectStoreBitsHandler>();
    definition.extensions.freeze();
    definition.capabilities.freeze();
    const AddressSpaceId id{definition.id};
    addressSpaceNames_.emplace(definition.name, id);
    addressSpaces_.emplace(id, AddressSpaceRecord{std::move(definition), std::move(accessBackend)});
    return id;
}

/**
 * @brief Creates a value through `createAddressSpace`.
 *
 * @param name Value supplied for `name`.
 * @param bitSize Value supplied for `bitSize`.
 * @param accessBackend Value supplied for `accessBackend`.
 * @param extensions Value supplied for `extensions`.
 * @param capabilities Value supplied for `capabilities`.
 * @return Value produced by the operation.
 */
AddressSpaceId MemoryController::createAddressSpace(std::string name, std::size_t bitSize, std::unique_ptr<BitAccess> accessBackend, types::ExtensionSet extensions, MemoryCapabilitySet capabilities) {
    AddressSpaceDefinition definition{};
    definition.name = std::move(name);
    definition.bitSize = bitSize;
    definition.extensions = std::move(extensions);
    definition.capabilities = std::move(capabilities);
    return registerAddressSpace(std::move(definition), std::move(accessBackend));
}

/**
 * @brief Checks the condition represented by `hasAddressSpace`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
bool MemoryController::hasAddressSpace(AddressSpaceId id) const noexcept {
    return addressSpaces_.find(id) != addressSpaces_.end();
}

/**
 * @brief Finds the value requested by `findAddressSpace`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
const AddressSpaceDefinition *MemoryController::findAddressSpace(AddressSpaceId id) const noexcept {
    const auto it{addressSpaces_.find(id)};
    return it == addressSpaces_.end() ? nullptr : &it->second.definition;
}

/**
 * @brief Returns the value required by `requireAddressSpace`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
const AddressSpaceDefinition &MemoryController::requireAddressSpace(AddressSpaceId id) const {
    return requireAddressSpaceRecord(id).definition;
}

/**
 * @brief Returns the value required by `requireAddressSpaceRecord`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
MemoryController::AddressSpaceRecord &MemoryController::requireAddressSpaceRecord(AddressSpaceId id) {
    const auto it{addressSpaces_.find(id)};
    if (it == addressSpaces_.end())
        throw MemoryError{MemoryErrorCode::UnknownAddressSpace, "MemoryController: unknown address space " + idText(id.value)};
    return it->second;
}

/**
 * @brief Returns the value required by `requireAddressSpaceRecord`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
const MemoryController::AddressSpaceRecord &MemoryController::requireAddressSpaceRecord(AddressSpaceId id) const {
    const auto it{addressSpaces_.find(id)};
    if (it == addressSpaces_.end())
        throw MemoryError{MemoryErrorCode::UnknownAddressSpace, "MemoryController: unknown address space " + idText(id.value)};
    return it->second;
}

/**
 * @brief Returns the value exposed by `access`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
const BitAccess &MemoryController::access(AddressSpaceId id) const {
    return *requireAddressSpaceRecord(id).access;
}

/**
 * @brief Returns the value exposed by `access`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
BitAccess &MemoryController::access(AddressSpaceId id) {
    return *requireAddressSpaceRecord(id).access;
}

/**
 * @brief Validates data through `validateAddressRange`.
 *
 * @param range Value supplied for `range`.
 * @param owner Value supplied for `owner`.
 */
void MemoryController::validateAddressRange(const AddressRange &range, const char *owner) const {
    if (!range.begin.space.valid())
        throw MemoryError{MemoryErrorCode::UnknownAddressSpace, std::string{owner} + ": address has no address space"};
    const AddressSpaceDefinition &space{requireAddressSpace(range.begin.space)};
    const std::size_t end{checkedAdd(range.begin.bitOffset, range.bitSize, owner)};
    if (range.begin.bitOffset > space.bitSize || end > space.bitSize)
        throw MemoryError{MemoryErrorCode::OutOfBounds, std::string{owner} + ": range escapes address space '" + space.name + "'"};
}

/**
 * @brief Creates a value through `createRegion`.
 *
 * @param name Value supplied for `name`.
 * @param range Value supplied for `range`.
 * @param extensions Value supplied for `extensions`.
 * @param capabilities Value supplied for `capabilities`.
 * @return Value produced by the operation.
 */
RegionId MemoryController::createRegion(std::string name, AddressRange range, types::ExtensionSet extensions, MemoryCapabilitySet capabilities) {
    if (name.empty())
        throw MemoryError{MemoryErrorCode::InvalidArgument, "MemoryController::createRegion: name cannot be empty"};
    if (range.bitSize == 0)
        throw MemoryError{MemoryErrorCode::InvalidArgument, "MemoryController::createRegion: region size must be greater than zero"};
    validateAddressRange(range, "MemoryController::createRegion");

    MemoryRegion region{};
    region.id = nextRegionId();
    region.name = std::move(name);
    region.range = range;
    region.extensions = std::move(extensions);
    region.capabilities = std::move(capabilities);
    region.extensions.freeze();
    region.capabilities.freeze();

    const RegionId id{region.id};
    regions_.emplace(id, std::move(region));
    allocationStrategies_.emplace(id, std::make_unique<LinearAllocationStrategy>());
    return id;
}

/**
 * @brief Sets the value handled by `setAllocationStrategy`.
 *
 * @param region Value supplied for `region`.
 * @param strategy Value supplied for `strategy`.
 */
void MemoryController::setAllocationStrategy(RegionId region, std::unique_ptr<AllocationStrategy> strategy) {
    (void)requireRegion(region);
    if (!strategy)
        throw MemoryError{MemoryErrorCode::InvalidArgument, "MemoryController::setAllocationStrategy: strategy cannot be null"};
    allocationStrategies_.insert_or_assign(region, std::move(strategy));
}

/**
 * @brief Checks the condition represented by `hasRegion`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
bool MemoryController::hasRegion(RegionId id) const noexcept {
    return regions_.find(id) != regions_.end();
}

/**
 * @brief Finds the value requested by `findRegion`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
const MemoryRegion *MemoryController::findRegion(RegionId id) const noexcept {
    const auto it{regions_.find(id)};
    return it == regions_.end() ? nullptr : &it->second;
}

/**
 * @brief Returns the value required by `requireRegion`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
const MemoryRegion &MemoryController::requireRegion(RegionId id) const {
    const auto it{regions_.find(id)};
    if (it == regions_.end())
        throw MemoryError{MemoryErrorCode::UnknownRegion, "MemoryController: unknown region " + idText(id.value)};
    return it->second;
}

/**
 * @brief Returns the value required by `requireRegionMutable`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
MemoryRegion &MemoryController::requireRegionMutable(RegionId id) {
    const auto it{regions_.find(id)};
    if (it == regions_.end())
        throw MemoryError{MemoryErrorCode::UnknownRegion, "MemoryController: unknown region " + idText(id.value)};
    return it->second;
}

/**
 * @brief Checks the condition represented by `activeAllocationsIn`.
 *
 * @param range Value supplied for `range`.
 * @return Value produced by the operation.
 */
std::vector<Allocation> MemoryController::activeAllocationsIn(const AddressRange &range) const {
    std::vector<Allocation> result;
    for (const auto &[id, allocation] : allocations_) {
        (void)id;
        if (allocation.active() && allocation.range.begin.space == range.begin.space && allocation.range.overlaps(range))
            result.push_back(allocation);
    }
    return result;
}

/**
 * @brief Allocates storage through `allocateBits`.
 *
 * @param regionId Value supplied for `regionId`.
 * @param bitSize Value supplied for `bitSize`.
 * @param alignmentBits Value supplied for `alignmentBits`.
 * @param lifetime Value supplied for `lifetime`.
 * @param extensions Value supplied for `extensions`.
 * @param capabilities Value supplied for `capabilities`.
 * @return Value produced by the operation.
 */
Allocation MemoryController::allocateBits(RegionId regionId, std::size_t bitSize, std::size_t alignmentBits, std::optional<LifetimeId> lifetime, types::ExtensionSet extensions, MemoryCapabilitySet capabilities) {
    if (bitSize == 0)
        throw MemoryError{MemoryErrorCode::InvalidArgument, "MemoryController::allocateBits: allocation size must be greater than zero"};
    validateAlignment(alignmentBits, "MemoryController::allocateBits");

    const MemoryRegion &region{requireRegion(regionId)};
    if (lifetime && !lifetimeAlive(*lifetime))
        throw MemoryError{MemoryErrorCode::DeadLifetime, "MemoryController::allocateBits: allocation lifetime is not alive"};

    auto strategyIt{allocationStrategies_.find(regionId)};
    if (strategyIt == allocationStrategies_.end() || !strategyIt->second)
        throw MemoryError{MemoryErrorCode::InvalidArgument, "MemoryController::allocateBits: region has no allocation strategy"};

    const auto existing{activeAllocationsIn(region.range)};
    const AllocationRequest request{region.range, bitSize, alignmentBits};
    const auto placement{strategyIt->second->place(request, existing)};
    if (!placement)
        throw MemoryError{MemoryErrorCode::OutOfMemory, "MemoryController::allocateBits: allocation strategy found no placement"};

    if (placement->regionRelativeBitOffset > region.range.bitSize)
        throw MemoryError{MemoryErrorCode::OutOfBounds, "MemoryController::allocateBits: strategy placement starts outside the region"};

    const std::size_t absoluteOffset{
        checkedAdd(region.range.begin.bitOffset, placement->regionRelativeBitOffset, "MemoryController::allocateBits")
    };
    const AddressRange range{Address{region.range.begin.space, absoluteOffset}, bitSize};
    if (!region.range.contains(range))
        throw MemoryError{MemoryErrorCode::OutOfBounds, "MemoryController::allocateBits: strategy placement escapes the region"};
    if (!aligned(absoluteOffset, alignmentBits))
        throw MemoryError{MemoryErrorCode::InvalidAlignment, "MemoryController::allocateBits: strategy returned a misaligned placement"};

    Allocation allocation{};
    allocation.id = nextAllocationId();
    allocation.region = regionId;
    allocation.range = range;
    allocation.alignmentBits = alignmentBits;
    allocation.lifetime = lifetime;
    allocation.extensions = std::move(extensions);
    allocation.capabilities = std::move(capabilities);
    allocation.extensions.freeze();
    allocation.capabilities.freeze();

    const AllocationId id{allocation.id};
    allocations_.emplace(id, allocation);
    return allocation;
}

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
MemoryReference MemoryController::allocate(RegionId region, types::TypeId type, std::optional<LifetimeId> lifetime, types::ExtensionSet extensions, MemoryCapabilitySet capabilities) {
    const types::TypeId canonical{types_->canonical(std::move(type))};
    if (!types_->hasType(canonical))
        throw MemoryError{MemoryErrorCode::InvalidReference, "MemoryController::allocate: unknown type '" + canonical.name + "'"};
    const types::TypeLayout layout{layouts_->compute(canonical)};
    const Allocation allocation{allocateBits(
        region, layout.bitSize, layout.alignmentBits, lifetime, std::move(extensions), std::move(capabilities)
    )};
    return MemoryReference{allocation.range.begin, canonical, allocation.id};
}

/**
 * @brief Releases or removes data through `release`.
 *
 * @param id Value supplied for `id`.
 */
void MemoryController::release(AllocationId id) {
    auto it{allocations_.find(id)};
    if (it == allocations_.end())
        throw MemoryError{MemoryErrorCode::UnknownAllocation, "MemoryController::release: unknown allocation " + idText(id.value)};
    if (!it->second.active())
        throw MemoryError{MemoryErrorCode::ReleasedAllocation, "MemoryController::release: allocation is already released"};
    it->second.state = AllocationState::Released;
}

/**
 * @brief Finds the value requested by `findAllocation`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
const Allocation *MemoryController::findAllocation(AllocationId id) const noexcept {
    const auto it{allocations_.find(id)};
    return it == allocations_.end() ? nullptr : &it->second;
}

/**
 * @brief Returns the value required by `requireAllocation`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
const Allocation &MemoryController::requireAllocation(AllocationId id) const {
    const auto *allocation{findAllocation(id)};
    if (!allocation)
        throw MemoryError{MemoryErrorCode::UnknownAllocation, "MemoryController: unknown allocation " + idText(id.value)};
    return *allocation;
}

/**
 * @brief Returns the value required by `requireAllocationMutable`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
Allocation &MemoryController::requireAllocationMutable(AllocationId id) {
    const auto it{allocations_.find(id)};
    if (it == allocations_.end())
        throw MemoryError{MemoryErrorCode::UnknownAllocation, "MemoryController: unknown allocation " + idText(id.value)};
    return it->second;
}

/**
 * @brief Returns the value exposed by `allocationActive`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
bool MemoryController::allocationActive(AllocationId id) const {
    return requireAllocation(id).active();
}

/**
 * @brief Starts the operation represented by `beginLifetime`.
 *
 * @param extensions Value supplied for `extensions`.
 * @return Value produced by the operation.
 */
LifetimeId MemoryController::beginLifetime(types::ExtensionSet extensions) {
    Lifetime lifetime{};
    lifetime.id = nextLifetimeId();
    lifetime.extensions = std::move(extensions);
    lifetime.extensions.freeze();
    const LifetimeId id{lifetime.id};
    lifetimes_.emplace(id, std::move(lifetime));
    return id;
}

/**
 * @brief Completes the operation represented by `endLifetime`.
 *
 * @param id Value supplied for `id`.
 */
void MemoryController::endLifetime(LifetimeId id) {
    auto it{lifetimes_.find(id)};
    if (it == lifetimes_.end())
        throw MemoryError{MemoryErrorCode::UnknownLifetime, "MemoryController::endLifetime: unknown lifetime " + idText(id.value)};
    it->second.alive = false;
}

/**
 * @brief Implements the `lifetimeAlive` operation.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
bool MemoryController::lifetimeAlive(LifetimeId id) const {
    return requireLifetime(id).alive;
}

/**
 * @brief Finds the value requested by `findLifetime`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
const Lifetime *MemoryController::findLifetime(LifetimeId id) const noexcept {
    const auto it{lifetimes_.find(id)};
    return it == lifetimes_.end() ? nullptr : &it->second;
}

/**
 * @brief Returns the value required by `requireLifetime`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
const Lifetime &MemoryController::requireLifetime(LifetimeId id) const {
    const auto *lifetime{findLifetime(id)};
    if (!lifetime)
        throw MemoryError{MemoryErrorCode::UnknownLifetime, "MemoryController: unknown lifetime " + idText(id.value)};
    return *lifetime;
}

/**
 * @brief Implements the `reference` operation.
 *
 * @param addressValue Value supplied for `addressValue`.
 * @param type Value supplied for `type`.
 * @param provenance Value supplied for `provenance`.
 * @return Value produced by the operation.
 */
MemoryReference MemoryController::reference(Address addressValue, types::TypeId type, std::optional<AllocationId> provenance) const {
    MemoryReference result{addressValue, types_->canonical(std::move(type)), provenance};
    validateReference(result);
    return result;
}

/**
 * @brief Implements the `dereference` operation.
 *
 * @param referenceValue Value supplied for `referenceValue`.
 * @return Value produced by the operation.
 */
AddressRange MemoryController::dereference(const MemoryReference &referenceValue) const {
    if (!referenceValue.type.valid() || !types_->hasType(referenceValue.type))
        throw MemoryError{MemoryErrorCode::InvalidReference, "MemoryController::dereference: unknown or empty type"};

    const types::TypeLayout layout{layouts_->compute(referenceValue.type)};
    const AddressRange typedRange{referenceValue.address, layout.bitSize};
    validateAddressRange(typedRange, "MemoryController::dereference");

    if (!referenceValue.provenance)
        return typedRange;

    const Allocation &allocation{requireAllocation(*referenceValue.provenance)};
    if (!allocation.active())
        throw MemoryError{MemoryErrorCode::ReleasedAllocation, "MemoryController::dereference: provenance allocation has been released"};
    if (!allocation.range.contains(typedRange))
        throw MemoryError{MemoryErrorCode::InvalidReference, "MemoryController::dereference: typed range escapes provenance allocation"};
    if (allocation.lifetime && !lifetimeAlive(*allocation.lifetime))
        throw MemoryError{MemoryErrorCode::DeadLifetime, "MemoryController::dereference: provenance lifetime has ended"};
    return typedRange;
}

/**
 * @brief Validates data through `validateReference`.
 *
 * @param referenceValue Value supplied for `referenceValue`.
 */
void MemoryController::validateReference(const MemoryReference &referenceValue) const {
    (void)dereference(referenceValue);
}

/**
 * @brief Loads data through `loadBits`.
 *
 * @param addressValue Value supplied for `addressValue`.
 * @param bitSize Value supplied for `bitSize`.
 * @return Value produced by the operation.
 */
types::BitValue MemoryController::loadBits(Address addressValue, std::size_t bitSize) const {
    const AddressRange range{addressValue, bitSize};
    validateAddressRange(range, "MemoryController::loadBits");
    return invoke<LoadBitsOperation>(addressValue.space, LoadBitsOperation::Request{addressValue, bitSize});
}

/**
 * @brief Stores data through `storeBits`.
 *
 * @param addressValue Value supplied for `addressValue`.
 * @param value Value supplied for `value`.
 */
void MemoryController::storeBits(Address addressValue, const types::BitValue &value) {
    const AddressRange range{addressValue, value.bitSize()};
    validateAddressRange(range, "MemoryController::storeBits");
    invoke<StoreBitsOperation>(addressValue.space, StoreBitsOperation::Request{addressValue, value});
}

/**
 * @brief Implements the `targetFor` operation.
 *
 * @param referenceValue Value supplied for `referenceValue`.
 * @return Value produced by the operation.
 */
MemoryTarget MemoryController::targetFor(const MemoryReference &referenceValue) noexcept {
    if (referenceValue.provenance)
        return *referenceValue.provenance;
    return referenceValue.address.space;
}

/**
 * @brief Loads data through `load`.
 *
 * @param referenceValue Value supplied for `referenceValue`.
 * @return Value produced by the operation.
 */
types::BitValue MemoryController::load(const MemoryReference &referenceValue) const {
    const AddressRange range{dereference(referenceValue)};

    // StorageController's stable public API is BitStorage-based. Materialize
    // exactly this value's physical bits so existing StorageCapability behavior
    // remains the single authority for type encoding without coupling Memory to it.
    const types::BitValue physical{invoke<LoadBitsOperation>(
        targetFor(referenceValue), LoadBitsOperation::Request{range.begin, range.bitSize}
    )};
    types::BitStorage temporary{range.bitSize};
    temporary.storeBits(types::BitAddress{0}, physical);
    return storage_->load(referenceValue.type, temporary, types::BitAddress{0});
}

/**
 * @brief Stores data through `store`.
 *
 * @param referenceValue Value supplied for `referenceValue`.
 * @param value Value supplied for `value`.
 */
void MemoryController::store(const MemoryReference &referenceValue, const types::BitValue &value) {
    const AddressRange range{dereference(referenceValue)};

    // Encode through the existing type storage semantics, then commit the exact
    // physical bits to the address-space backend. This preserves the existing storage
    // API and keeps custom Memory backends independent from type behavior.
    types::BitStorage temporary{range.bitSize};
    storage_->store(referenceValue.type, temporary, types::BitAddress{0}, value);
    const types::BitValue physical{temporary.loadBits(types::BitAddress{0}, range.bitSize)};

    invoke<StoreBitsOperation>(targetFor(referenceValue), StoreBitsOperation::Request{range.begin, physical});
}

} // namespace novac::assets::memory
