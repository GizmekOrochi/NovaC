#include "novac/assets/memory/MemoryController.hpp"

#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace novac::assets::memory {

MemoryController::MemoryController(const types::TypeController &types, const types::LayoutController &layouts, const types::StorageController &storage)
     : types_{&types}, layouts_{&layouts}, storage_{&storage} {}

MemoryCapabilitySet &MemoryController::capabilities() noexcept { return globalCapabilities_; }
const MemoryCapabilitySet &MemoryController::capabilities() const noexcept { return globalCapabilities_; }

namespace {

std::string idText(std::size_t value) {
    return std::to_string(value);
}

} // namespace

std::size_t MemoryController::checkedAdd(std::size_t left, std::size_t right, const char *owner) {
    if (left > std::numeric_limits<std::size_t>::max() - right)
        throw MemoryError{MemoryErrorCode::OutOfBounds, std::string{owner} + ": bit offset overflow"};
    return left + right;
}

void MemoryController::validateAlignment(std::size_t alignmentBits, const char *owner) {
    if (alignmentBits == 0)
        throw MemoryError{MemoryErrorCode::InvalidAlignment, std::string{owner} + ": alignment must be at least one bit"};
}

bool MemoryController::aligned(std::size_t bitOffset, std::size_t alignmentBits) noexcept {
    return alignmentBits != 0 && bitOffset % alignmentBits == 0;
}

AddressSpaceId MemoryController::nextAddressSpaceId() {
    if (nextAddressSpaceValue_ == 0)
        throw MemoryError{MemoryErrorCode::InvalidArgument, "MemoryController: address-space id space exhausted"};
    const AddressSpaceId id{nextAddressSpaceValue_++};
    if (nextAddressSpaceValue_ == 0 && id.value != std::numeric_limits<std::size_t>::max())
        throw MemoryError{MemoryErrorCode::InvalidArgument, "MemoryController: address-space id overflow"};
    return id;
}

RegionId MemoryController::nextRegionId() {
    if (nextRegionValue_ == 0)
        throw MemoryError{MemoryErrorCode::InvalidArgument, "MemoryController: region id space exhausted"};
    return RegionId{nextRegionValue_++};
}

AllocationId MemoryController::nextAllocationId() {
    if (nextAllocationValue_ == 0)
        throw MemoryError{MemoryErrorCode::InvalidArgument, "MemoryController: allocation id space exhausted"};
    return AllocationId{nextAllocationValue_++};
}

LifetimeId MemoryController::nextLifetimeId() {
    if (nextLifetimeValue_ == 0)
        throw MemoryError{MemoryErrorCode::InvalidArgument, "MemoryController: lifetime id space exhausted"};
    return LifetimeId{nextLifetimeValue_++};
}

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

AddressSpaceId MemoryController::createAddressSpace(std::string name, std::size_t bitSize, std::unique_ptr<BitAccess> accessBackend, types::ExtensionSet extensions, MemoryCapabilitySet capabilities) {
    AddressSpaceDefinition definition{};
    definition.name = std::move(name);
    definition.bitSize = bitSize;
    definition.extensions = std::move(extensions);
    definition.capabilities = std::move(capabilities);
    return registerAddressSpace(std::move(definition), std::move(accessBackend));
}

bool MemoryController::hasAddressSpace(AddressSpaceId id) const noexcept {
    return addressSpaces_.find(id) != addressSpaces_.end();
}

const AddressSpaceDefinition *MemoryController::findAddressSpace(AddressSpaceId id) const noexcept {
    const auto it{addressSpaces_.find(id)};
    return it == addressSpaces_.end() ? nullptr : &it->second.definition;
}

const AddressSpaceDefinition &MemoryController::requireAddressSpace(AddressSpaceId id) const {
    return requireAddressSpaceRecord(id).definition;
}

MemoryController::AddressSpaceRecord &MemoryController::requireAddressSpaceRecord(AddressSpaceId id) {
    const auto it{addressSpaces_.find(id)};
    if (it == addressSpaces_.end())
        throw MemoryError{MemoryErrorCode::UnknownAddressSpace, "MemoryController: unknown address space " + idText(id.value)};
    return it->second;
}

const MemoryController::AddressSpaceRecord &MemoryController::requireAddressSpaceRecord(AddressSpaceId id) const {
    const auto it{addressSpaces_.find(id)};
    if (it == addressSpaces_.end())
        throw MemoryError{MemoryErrorCode::UnknownAddressSpace, "MemoryController: unknown address space " + idText(id.value)};
    return it->second;
}

const BitAccess &MemoryController::access(AddressSpaceId id) const {
    return *requireAddressSpaceRecord(id).access;
}

BitAccess &MemoryController::access(AddressSpaceId id) {
    return *requireAddressSpaceRecord(id).access;
}

void MemoryController::validateAddressRange(const AddressRange &range, const char *owner) const {
    if (!range.begin.space.valid())
        throw MemoryError{MemoryErrorCode::UnknownAddressSpace, std::string{owner} + ": address has no address space"};
    const AddressSpaceDefinition &space{requireAddressSpace(range.begin.space)};
    const std::size_t end{checkedAdd(range.begin.bitOffset, range.bitSize, owner)};
    if (range.begin.bitOffset > space.bitSize || end > space.bitSize)
        throw MemoryError{MemoryErrorCode::OutOfBounds, std::string{owner} + ": range escapes address space '" + space.name + "'"};
}

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

void MemoryController::setAllocationStrategy(RegionId region, std::unique_ptr<AllocationStrategy> strategy) {
    (void)requireRegion(region);
    if (!strategy)
        throw MemoryError{MemoryErrorCode::InvalidArgument, "MemoryController::setAllocationStrategy: strategy cannot be null"};
    allocationStrategies_.insert_or_assign(region, std::move(strategy));
}

bool MemoryController::hasRegion(RegionId id) const noexcept {
    return regions_.find(id) != regions_.end();
}

const MemoryRegion *MemoryController::findRegion(RegionId id) const noexcept {
    const auto it{regions_.find(id)};
    return it == regions_.end() ? nullptr : &it->second;
}

const MemoryRegion &MemoryController::requireRegion(RegionId id) const {
    const auto it{regions_.find(id)};
    if (it == regions_.end())
        throw MemoryError{MemoryErrorCode::UnknownRegion, "MemoryController: unknown region " + idText(id.value)};
    return it->second;
}

MemoryRegion &MemoryController::requireRegionMutable(RegionId id) {
    const auto it{regions_.find(id)};
    if (it == regions_.end())
        throw MemoryError{MemoryErrorCode::UnknownRegion, "MemoryController: unknown region " + idText(id.value)};
    return it->second;
}

std::vector<Allocation> MemoryController::activeAllocationsIn(const AddressRange &range) const {
    std::vector<Allocation> result;
    for (const auto &[id, allocation] : allocations_) {
        (void)id;
        if (allocation.active() && allocation.range.begin.space == range.begin.space && allocation.range.overlaps(range))
            result.push_back(allocation);
    }
    return result;
}

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

void MemoryController::release(AllocationId id) {
    auto it{allocations_.find(id)};
    if (it == allocations_.end())
        throw MemoryError{MemoryErrorCode::UnknownAllocation, "MemoryController::release: unknown allocation " + idText(id.value)};
    if (!it->second.active())
        throw MemoryError{MemoryErrorCode::ReleasedAllocation, "MemoryController::release: allocation is already released"};
    it->second.state = AllocationState::Released;
}

const Allocation *MemoryController::findAllocation(AllocationId id) const noexcept {
    const auto it{allocations_.find(id)};
    return it == allocations_.end() ? nullptr : &it->second;
}

const Allocation &MemoryController::requireAllocation(AllocationId id) const {
    const auto *allocation{findAllocation(id)};
    if (!allocation)
        throw MemoryError{MemoryErrorCode::UnknownAllocation, "MemoryController: unknown allocation " + idText(id.value)};
    return *allocation;
}

Allocation &MemoryController::requireAllocationMutable(AllocationId id) {
    const auto it{allocations_.find(id)};
    if (it == allocations_.end())
        throw MemoryError{MemoryErrorCode::UnknownAllocation, "MemoryController: unknown allocation " + idText(id.value)};
    return it->second;
}

bool MemoryController::allocationActive(AllocationId id) const {
    return requireAllocation(id).active();
}

LifetimeId MemoryController::beginLifetime(types::ExtensionSet extensions) {
    Lifetime lifetime{};
    lifetime.id = nextLifetimeId();
    lifetime.extensions = std::move(extensions);
    lifetime.extensions.freeze();
    const LifetimeId id{lifetime.id};
    lifetimes_.emplace(id, std::move(lifetime));
    return id;
}

void MemoryController::endLifetime(LifetimeId id) {
    auto it{lifetimes_.find(id)};
    if (it == lifetimes_.end())
        throw MemoryError{MemoryErrorCode::UnknownLifetime, "MemoryController::endLifetime: unknown lifetime " + idText(id.value)};
    it->second.alive = false;
}

bool MemoryController::lifetimeAlive(LifetimeId id) const {
    return requireLifetime(id).alive;
}

const Lifetime *MemoryController::findLifetime(LifetimeId id) const noexcept {
    const auto it{lifetimes_.find(id)};
    return it == lifetimes_.end() ? nullptr : &it->second;
}

const Lifetime &MemoryController::requireLifetime(LifetimeId id) const {
    const auto *lifetime{findLifetime(id)};
    if (!lifetime)
        throw MemoryError{MemoryErrorCode::UnknownLifetime, "MemoryController: unknown lifetime " + idText(id.value)};
    return *lifetime;
}

MemoryReference MemoryController::reference(Address addressValue, types::TypeId type, std::optional<AllocationId> provenance) const {
    MemoryReference result{addressValue, types_->canonical(std::move(type)), provenance};
    validateReference(result);
    return result;
}

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

void MemoryController::validateReference(const MemoryReference &referenceValue) const {
    (void)dereference(referenceValue);
}

types::BitValue MemoryController::loadBits(Address addressValue, std::size_t bitSize) const {
    const AddressRange range{addressValue, bitSize};
    validateAddressRange(range, "MemoryController::loadBits");
    return invoke<LoadBitsOperation>(addressValue.space, LoadBitsOperation::Request{addressValue, bitSize});
}

void MemoryController::storeBits(Address addressValue, const types::BitValue &value) {
    const AddressRange range{addressValue, value.bitSize()};
    validateAddressRange(range, "MemoryController::storeBits");
    invoke<StoreBitsOperation>(addressValue.space, StoreBitsOperation::Request{addressValue, value});
}

MemoryTarget MemoryController::targetFor(const MemoryReference &referenceValue) noexcept {
    if (referenceValue.provenance)
        return *referenceValue.provenance;
    return referenceValue.address.space;
}

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
