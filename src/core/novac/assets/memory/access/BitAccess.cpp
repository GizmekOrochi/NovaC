#include "novac/assets/memory/access/BitAccess.hpp"

namespace novac::assets::memory {

/**
 * @brief Constructs a `BitStorageAccess` instance.
 *
 * @param bitSize Value supplied for `bitSize`.
 */
BitStorageAccess::BitStorageAccess(std::size_t bitSize) : storage_{bitSize} {}

/**
 * @brief Implements the `bitSize` operation.
 *
 * @return Value produced by the operation.
 */
std::size_t BitStorageAccess::bitSize() const noexcept { return storage_.bitSize(); }

/**
 * @brief Loads data through `loadBits`.
 *
 * @param bitOffset Value supplied for `bitOffset`.
 * @param bitSize Value supplied for `bitSize`.
 * @return Value produced by the operation.
 */
types::BitValue BitStorageAccess::loadBits(std::size_t bitOffset, std::size_t bitSize) const {
    return storage_.loadBits(types::BitAddress{bitOffset}, bitSize);
}

/**
 * @brief Stores data through `storeBits`.
 *
 * @param bitOffset Value supplied for `bitOffset`.
 * @param value Value supplied for `value`.
 */
void BitStorageAccess::storeBits(std::size_t bitOffset, const types::BitValue &value) {
    storage_.storeBits(types::BitAddress{bitOffset}, value);
}

/**
 * @brief Returns the value exposed by `storage`.
 *
 * @return Value produced by the operation.
 */
const types::BitStorage &BitStorageAccess::storage() const noexcept { return storage_; }
/**
 * @brief Returns the value exposed by `storage`.
 *
 * @return Value produced by the operation.
 */
types::BitStorage &BitStorageAccess::storage() noexcept { return storage_; }

} // namespace novac::assets::memory
