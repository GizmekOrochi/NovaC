#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

namespace novac::assets::types {

/** Bit-addressed location inside a storage object. */
struct BitAddress {
    std::size_t bitOffset{0};

    /**
     * @brief Performs the `advanced` operation.
     *
     * @param bits Value supplied for `bits`.
     * @return Value produced by the operation.
     */
    BitAddress advanced(std::size_t bits) const {
        if (bitOffset > static_cast<std::size_t>(-1) - bits)
            throw std::runtime_error("BitAddress::advanced: address overflow");
        return BitAddress{bitOffset + bits};
    }
};

/** Owned bit sequence used by the low-level storage API. */
class BitValue final {
public:
    /**
     * @brief Constructs a `BitValue` instance.
     *
     * @param bitSize Value supplied for `bitSize`.
     */
    explicit BitValue(std::size_t bitSize = 0);

    /**
     * @brief Performs the `fromUnsigned` operation.
     *
     * @param value Value supplied for `value`.
     * @param bitSize Value supplied for `bitSize`.
     * @return Value produced by the operation.
     */
    static BitValue fromUnsigned(std::uint64_t value, std::size_t bitSize);

    /**
     * @brief Performs the `bitSize` operation.
     *
     * @return Value produced by the operation.
     */
    std::size_t bitSize() const noexcept { return bitSize_; }
    /**
     * @brief Performs the `byteSize` operation.
     *
     * @return Value produced by the operation.
     */
    std::size_t byteSize() const noexcept { return bytes_.size(); }
    /**
     * @brief Checks the condition represented by `empty`.
     *
     * @return Value produced by the operation.
     */
    bool empty() const noexcept { return bitSize_ == 0; }

    /**
     * @brief Performs the `bit` operation.
     *
     * @param index Value supplied for `index`.
     * @return Value produced by the operation.
     */
    bool bit(std::size_t index) const;
    /**
     * @brief Sets the value handled by `setBit`.
     *
     * @param index Value supplied for `index`.
     * @param value Value supplied for `value`.
     */
    void setBit(std::size_t index, bool value);

    /**
     * @brief Performs the `toUnsigned` operation.
     *
     * @return Value produced by the operation.
     */
    std::uint64_t toUnsigned() const;
    /**
     * @brief Performs the `extract` operation.
     *
     * @param bitOffset Value supplied for `bitOffset`.
     * @param bitSize Value supplied for `bitSize`.
     * @return Value produced by the operation.
     */
    BitValue extract(std::size_t bitOffset, std::size_t bitSize) const;
    /**
     * @brief Performs the `insert` operation.
     *
     * @param bitOffset Value supplied for `bitOffset`.
     * @param value Value supplied for `value`.
     */
    void insert(std::size_t bitOffset, const BitValue &value);
    /**
     * @brief Performs the `shiftedLeft` operation.
     *
     * @param count Value supplied for `count`.
     * @return Value produced by the operation.
     */
    BitValue shiftedLeft(std::size_t count) const;
    /**
     * @brief Performs the `shiftedRight` operation.
     *
     * @param count Value supplied for `count`.
     * @return Value produced by the operation.
     */
    BitValue shiftedRight(std::size_t count) const;
    /**
     * @brief Performs the `masked` operation.
     *
     * @param mask Value supplied for `mask`.
     * @return Value produced by the operation.
     */
    BitValue masked(const BitValue &mask) const;

    /**
     * @brief Performs the `bytes` operation.
     *
     * @return Value produced by the operation.
     */
    std::span<const std::uint8_t> bytes() const noexcept { return bytes_; }

private:
    /**
     * @brief Resets state through `clearUnusedHighBits`.
     */
    void clearUnusedHighBits() noexcept;

    std::size_t bitSize_{0};
    std::vector<std::uint8_t> bytes_{};
};

/** Mutable bit-addressed backing storage. Bit zero is the LSB of byte zero. */
class BitStorage final {
public:
    /**
     * @brief Constructs a `BitStorage` instance.
     *
     * @param bitSize Value supplied for `bitSize`.
     */
    explicit BitStorage(std::size_t bitSize = 0);

    /**
     * @brief Performs the `bitSize` operation.
     *
     * @return Value produced by the operation.
     */
    std::size_t bitSize() const noexcept { return bitSize_; }
    /**
     * @brief Performs the `byteSize` operation.
     *
     * @return Value produced by the operation.
     */
    std::size_t byteSize() const noexcept { return bytes_.size(); }

    /**
     * @brief Loads data through `loadBits`.
     *
     * @param address Value supplied for `address`.
     * @param bitSize Value supplied for `bitSize`.
     * @return Value produced by the operation.
     */
    BitValue loadBits(BitAddress address, std::size_t bitSize) const;
    /**
     * @brief Stores data through `storeBits`.
     *
     * @param address Value supplied for `address`.
     * @param value Value supplied for `value`.
     */
    void storeBits(BitAddress address, const BitValue &value);

    /**
     * @brief Performs the `bit` operation.
     *
     * @param index Value supplied for `index`.
     * @return Value produced by the operation.
     */
    bool bit(std::size_t index) const;
    /**
     * @brief Performs the `bytes` operation.
     *
     * @return Value produced by the operation.
     */
    std::span<const std::uint8_t> bytes() const noexcept { return bytes_; }

private:
    /**
     * @brief Validates data through `validateRange`.
     *
     * @param address Value supplied for `address`.
     * @param bitSize Value supplied for `bitSize`.
     * @param owner Value supplied for `owner`.
     */
    void validateRange(BitAddress address, std::size_t bitSize, const char *owner) const;

    std::size_t bitSize_{0};
    std::vector<std::uint8_t> bytes_{};
};

} // namespace novac::assets::types
