#pragma once

#include "novac/assets/memory/model/Memory.hpp"

namespace novac::assets::types {
class TypeController;
class LayoutController;
class StorageController;
}

namespace novac::assets::memory {

class BitAccess;
class MemoryController;

/** Controlled façade exposed to memory operation handlers. */
class MemoryContext final {
public:
    /**
     * @brief Returns the value exposed by `types`.
     *
     * @return Value produced by the operation.
     */
    const types::TypeController &types() const noexcept;
    /**
     * @brief Returns the value exposed by `layouts`.
     *
     * @return Value produced by the operation.
     */
    const types::LayoutController &layouts() const noexcept;
    /**
     * @brief Returns the value exposed by `storage`.
     *
     * @return Value produced by the operation.
     */
    const types::StorageController &storage() const noexcept;

    /**
     * @brief Returns the requested address-space definition.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    const AddressSpaceDefinition &addressSpace(AddressSpaceId id) const;
    /**
     * @brief Returns the value exposed by `region`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    const MemoryRegion &region(RegionId id) const;
    /**
     * @brief Returns the value exposed by `allocation`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    const Allocation &allocation(AllocationId id) const;

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

private:
    friend class MemoryController;
    /**
     * @brief Constructs a `MemoryContext` instance.
     *
     * @param controller Value supplied for `controller`.
     */
    explicit MemoryContext(MemoryController &controller) noexcept;
    /**
     * @brief Constructs a `MemoryContext` instance.
     *
     * @param controller Value supplied for `controller`.
     */
    explicit MemoryContext(const MemoryController &controller) noexcept;

    const MemoryController *controller_{nullptr};
    MemoryController *mutableController_{nullptr};
};

} // namespace novac::assets::memory
