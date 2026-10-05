#include "novac/assets/memory/access/BitAccess.hpp"

namespace novac::assets::memory {

BitStorageAccess::BitStorageAccess(std::size_t bitSize) : storage_{bitSize} {}

std::size_t BitStorageAccess::bitSize() const noexcept { return storage_.bitSize(); }

types::BitValue BitStorageAccess::loadBits(std::size_t bitOffset, std::size_t bitSize) const {
    return storage_.loadBits(types::BitAddress{bitOffset}, bitSize);
}

void BitStorageAccess::storeBits(std::size_t bitOffset, const types::BitValue &value) {
    storage_.storeBits(types::BitAddress{bitOffset}, value);
}

const types::BitStorage &BitStorageAccess::storage() const noexcept { return storage_; }
types::BitStorage &BitStorageAccess::storage() noexcept { return storage_; }

} // namespace novac::assets::memory
