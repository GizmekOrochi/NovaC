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
    const types::TypeController &types() const noexcept;
    const types::LayoutController &layouts() const noexcept;
    const types::StorageController &storage() const noexcept;

    const AddressSpaceDefinition &addressSpace(AddressSpaceId id) const;
    const MemoryRegion &region(RegionId id) const;
    const Allocation &allocation(AllocationId id) const;

    const BitAccess &access(AddressSpaceId id) const;
    BitAccess &access(AddressSpaceId id);

private:
    friend class MemoryController;
    explicit MemoryContext(MemoryController &controller) noexcept;
    explicit MemoryContext(const MemoryController &controller) noexcept;

    const MemoryController *controller_{nullptr};
    MemoryController *mutableController_{nullptr};
};

} // namespace novac::assets::memory
