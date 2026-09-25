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

    BitAddress advanced(std::size_t bits) const {
        if (bitOffset > static_cast<std::size_t>(-1) - bits)
            throw std::runtime_error("BitAddress::advanced: address overflow");
        return BitAddress{bitOffset + bits};
    }
};

/** Owned bit sequence used by the low-level storage API. */
class BitValue final {
public:
    explicit BitValue(std::size_t bitSize = 0);

    static BitValue fromUnsigned(std::uint64_t value, std::size_t bitSize);

    std::size_t bitSize() const noexcept { return bitSize_; }
    std::size_t byteSize() const noexcept { return bytes_.size(); }
    bool empty() const noexcept { return bitSize_ == 0; }

    bool bit(std::size_t index) const;
    void setBit(std::size_t index, bool value);

    std::uint64_t toUnsigned() const;
    BitValue extract(std::size_t bitOffset, std::size_t bitSize) const;
    void insert(std::size_t bitOffset, const BitValue &value);
    BitValue shiftedLeft(std::size_t count) const;
    BitValue shiftedRight(std::size_t count) const;
    BitValue masked(const BitValue &mask) const;

    std::span<const std::uint8_t> bytes() const noexcept { return bytes_; }

private:
    void clearUnusedHighBits() noexcept;

    std::size_t bitSize_{0};
    std::vector<std::uint8_t> bytes_{};
};

/** Mutable bit-addressed backing storage. Bit zero is the LSB of byte zero. */
class BitStorage final {
public:
    explicit BitStorage(std::size_t bitSize = 0);

    std::size_t bitSize() const noexcept { return bitSize_; }
    std::size_t byteSize() const noexcept { return bytes_.size(); }

    BitValue loadBits(BitAddress address, std::size_t bitSize) const;
    void storeBits(BitAddress address, const BitValue &value);

    bool bit(std::size_t index) const;
    std::span<const std::uint8_t> bytes() const noexcept { return bytes_; }

private:
    void validateRange(BitAddress address, std::size_t bitSize, const char *owner) const;

    std::size_t bitSize_{0};
    std::vector<std::uint8_t> bytes_{};
};

} // namespace novac::assets::types
