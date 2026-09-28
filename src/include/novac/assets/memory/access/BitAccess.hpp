#pragma once

#include "novac/assets/types/model/Storage.hpp"

#include <cstddef>

namespace novac::assets::memory {

/** Raw bit-access backend for an address space. */
class BitAccess {
public:
    virtual ~BitAccess() = default;
    virtual std::size_t bitSize() const noexcept = 0;
    virtual types::BitValue loadBits(std::size_t bitOffset, std::size_t bitSize) const = 0;
    virtual void storeBits(std::size_t bitOffset, const types::BitValue &value) = 0;
};

/** Standard in-memory BitAccess implementation backed by types::BitStorage. */
class BitStorageAccess final : public BitAccess {
public:
    explicit BitStorageAccess(std::size_t bitSize);

    std::size_t bitSize() const noexcept override;
    types::BitValue loadBits(std::size_t bitOffset, std::size_t bitSize) const override;
    void storeBits(std::size_t bitOffset, const types::BitValue &value) override;

    const types::BitStorage &storage() const noexcept;
    types::BitStorage &storage() noexcept;

private:
    types::BitStorage storage_;
};

} // namespace novac::assets::memory
