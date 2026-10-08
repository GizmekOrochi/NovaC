#include "novac/assets/memory/MemoryContext.hpp"

#include "novac/assets/memory/MemoryController.hpp"

namespace novac::assets::memory {

/**
 * @brief Constructs a `MemoryContext` instance.
 *
 * @param controller Value supplied for `controller`.
 */
MemoryContext::MemoryContext(MemoryController &controller) noexcept
    : controller_{&controller}, mutableController_{&controller} {}

/**
 * @brief Constructs a `MemoryContext` instance.
 *
 * @param controller Value supplied for `controller`.
 */
MemoryContext::MemoryContext(const MemoryController &controller) noexcept
    : controller_{&controller}, mutableController_{nullptr} {}

/**
 * @brief Returns the value exposed by `types`.
 *
 * @return Value produced by the operation.
 */
const types::TypeController &MemoryContext::types() const noexcept { return *controller_->types_; }
/**
 * @brief Returns the value exposed by `layouts`.
 *
 * @return Value produced by the operation.
 */
const types::LayoutController &MemoryContext::layouts() const noexcept { return *controller_->layouts_; }
/**
 * @brief Returns the value exposed by `storage`.
 *
 * @return Value produced by the operation.
 */
const types::StorageController &MemoryContext::storage() const noexcept { return *controller_->storage_; }

/**
 * @brief Returns the requested address-space definition.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
const AddressSpaceDefinition &MemoryContext::addressSpace(AddressSpaceId id) const {
    return controller_->requireAddressSpace(id);
}

/**
 * @brief Returns the value exposed by `region`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
const MemoryRegion &MemoryContext::region(RegionId id) const {
    return controller_->requireRegion(id);
}

/**
 * @brief Returns the value exposed by `allocation`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
const Allocation &MemoryContext::allocation(AllocationId id) const {
    return controller_->requireAllocation(id);
}

/**
 * @brief Returns the value exposed by `access`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
const BitAccess &MemoryContext::access(AddressSpaceId id) const {
    return static_cast<const MemoryController &>(*controller_).access(id);
}

/**
 * @brief Returns the value exposed by `access`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
BitAccess &MemoryContext::access(AddressSpaceId id) {
    if (!mutableController_)
        throw MemoryError{MemoryErrorCode::InvalidArgument, "MemoryContext: mutable access requested during a const operation"};
    return mutableController_->access(id);
}

} // namespace novac::assets::memory
