#pragma once

#include "novac/assets/types/model/Storage.hpp"

#include <cstddef>

namespace novac::assets::memory {

/** Raw bit-access backend for an address space. */
class BitAccess {
public:
    /**
     * @brief Destroys the `BitAccess` instance.
     */
    virtual ~BitAccess() = default;
    /**
     * @brief Performs the `bitSize` operation.
     *
     * @return Value produced by the operation.
     */
    virtual std::size_t bitSize() const noexcept = 0;
    /**
     * @brief Loads data through `loadBits`.
     *
     * @param bitOffset Value supplied for `bitOffset`.
     * @param bitSize Value supplied for `bitSize`.
     * @return Value produced by the operation.
     */
    virtual types::BitValue loadBits(std::size_t bitOffset, std::size_t bitSize) const = 0;
    /**
     * @brief Stores data through `storeBits`.
     *
     * @param bitOffset Value supplied for `bitOffset`.
     * @param value Value supplied for `value`.
     */
    virtual void storeBits(std::size_t bitOffset, const types::BitValue &value) = 0;
};

/** Standard in-memory BitAccess implementation backed by types::BitStorage. */
class BitStorageAccess final : public BitAccess {
public:
    /**
     * @brief Constructs a `BitStorageAccess` instance.
     *
     * @param bitSize Value supplied for `bitSize`.
     */
    explicit BitStorageAccess(std::size_t bitSize);

    /**
     * @brief Performs the `bitSize` operation.
     *
     * @return Value produced by the operation.
     */
    std::size_t bitSize() const noexcept override;
    /**
     * @brief Loads data through `loadBits`.
     *
     * @param bitOffset Value supplied for `bitOffset`.
     * @param bitSize Value supplied for `bitSize`.
     * @return Value produced by the operation.
     */
    types::BitValue loadBits(std::size_t bitOffset, std::size_t bitSize) const override;
    /**
     * @brief Stores data through `storeBits`.
     *
     * @param bitOffset Value supplied for `bitOffset`.
     * @param value Value supplied for `value`.
     */
    void storeBits(std::size_t bitOffset, const types::BitValue &value) override;

    /**
     * @brief Returns the value exposed by `storage`.
     *
     * @return Value produced by the operation.
     */
    const types::BitStorage &storage() const noexcept;
    /**
     * @brief Returns the value exposed by `storage`.
     *
     * @return Value produced by the operation.
     */
    types::BitStorage &storage() noexcept;

private:
    types::BitStorage storage_;
};

} // namespace novac::assets::memory
