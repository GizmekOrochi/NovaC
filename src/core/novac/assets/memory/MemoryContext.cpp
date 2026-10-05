#include "novac/assets/memory/MemoryContext.hpp"

#include "novac/assets/memory/MemoryController.hpp"

namespace novac::assets::memory {

MemoryContext::MemoryContext(MemoryController &controller) noexcept
    : controller_{&controller}, mutableController_{&controller} {}

MemoryContext::MemoryContext(const MemoryController &controller) noexcept
    : controller_{&controller}, mutableController_{nullptr} {}

const types::TypeController &MemoryContext::types() const noexcept { return *controller_->types_; }
const types::LayoutController &MemoryContext::layouts() const noexcept { return *controller_->layouts_; }
const types::StorageController &MemoryContext::storage() const noexcept { return *controller_->storage_; }

const AddressSpaceDefinition &MemoryContext::addressSpace(AddressSpaceId id) const {
    return controller_->requireAddressSpace(id);
}

const MemoryRegion &MemoryContext::region(RegionId id) const {
    return controller_->requireRegion(id);
}

const Allocation &MemoryContext::allocation(AllocationId id) const {
    return controller_->requireAllocation(id);
}

const BitAccess &MemoryContext::access(AddressSpaceId id) const {
    return static_cast<const MemoryController &>(*controller_).access(id);
}

BitAccess &MemoryContext::access(AddressSpaceId id) {
    if (!mutableController_)
        throw MemoryError{MemoryErrorCode::InvalidArgument, "MemoryContext: mutable access requested during a const operation"};
    return mutableController_->access(id);
}

} // namespace novac::assets::memory
