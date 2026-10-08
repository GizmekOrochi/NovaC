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
    /**
     * @brief Constructs a `StorageController` instance.
     *
     * @param types Value supplied for `types`.
     * @param layouts Value supplied for `layouts`.
     */
    StorageController(const TypeController &types, const LayoutController &layouts)
        : types_{types}, layouts_{layouts} {}

    /**
     * @brief Returns the value exposed by `types`.
     *
     * @return Value produced by the operation.
     */
    const TypeController &types() const noexcept override { return types_; }
    /**
     * @brief Performs the `layoutOf` operation.
     *
     * @param type Value supplied for `type`.
     * @return Value produced by the operation.
     */
    TypeLayout layoutOf(const TypeId &type) const override { return layouts_.compute(type); }

    /**
     * @brief Loads data through `loadBits`.
     *
     * @param storage Value supplied for `storage`.
     * @param address Value supplied for `address`.
     * @param bitSize Value supplied for `bitSize`.
     * @return Value produced by the operation.
     */
    BitValue loadBits(const BitStorage &storage, BitAddress address, std::size_t bitSize) const override;
    /**
     * @brief Stores data through `storeBits`.
     *
     * @param storage Value supplied for `storage`.
     * @param address Value supplied for `address`.
     * @param value Value supplied for `value`.
     */
    void storeBits(BitStorage &storage, BitAddress address, const BitValue &value) const override;

    /**
     * @brief Loads data through `load`.
     *
     * @param type Value supplied for `type`.
     * @param storage Value supplied for `storage`.
     * @param address Value supplied for `address`.
     * @return Value produced by the operation.
     */
    BitValue load(const TypeId &type, const BitStorage &storage, BitAddress address) const;
    /**
     * @brief Stores data through `store`.
     *
     * @param type Value supplied for `type`.
     * @param storage Value supplied for `storage`.
     * @param address Value supplied for `address`.
     * @param value Value supplied for `value`.
     */
    void store(const TypeId &type, BitStorage &storage, BitAddress address, const BitValue &value) const;

private:
    const TypeController &types_;
    const LayoutController &layouts_;
};

} // namespace novac::assets::types
