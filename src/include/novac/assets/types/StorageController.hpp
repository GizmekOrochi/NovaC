#pragma once

#include "novac/assets/types/LayoutController.hpp"
#include "novac/assets/types/model/Capabilities.hpp"
#include "novac/assets/types/model/Storage.hpp"

namespace novac::assets::types {

/**
 * Low-level bit-addressed storage service for NovaC types.
 *
 * StorageController materializes a type's StorageCapability when present and
 * otherwise performs direct bit-for-bit storage for the type's exact bit width. Custom capabilities receive a bounded StorageContext, so
 * their raw accesses cannot escape the current value's TypeLayout range.
 */
class StorageController final : public StorageContext {
public:
    StorageController(const TypeController &types, const LayoutController &layouts)
        : types_{types}, layouts_{layouts} {}

    const TypeController &types() const noexcept override { return types_; }
    TypeLayout layoutOf(const TypeId &type) const override { return layouts_.compute(type); }

    BitValue loadBits(const BitStorage &storage, BitAddress address, std::size_t bitSize) const override;
    void storeBits(BitStorage &storage, BitAddress address, const BitValue &value) const override;

    BitValue load(const TypeId &type, const BitStorage &storage, BitAddress address) const;
    void store(const TypeId &type, BitStorage &storage, BitAddress address, const BitValue &value) const;

private:
    const TypeController &types_;
    const LayoutController &layouts_;
};

} // namespace novac::assets::types
