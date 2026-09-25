#include "novac/assets/types/StorageController.hpp"

#include <limits>
#include <stdexcept>
#include <string>

namespace novac::assets::types {
namespace {

std::size_t byteCount(std::size_t bits) {
    if (bits > std::numeric_limits<std::size_t>::max() - 7U)
        throw std::runtime_error("bit storage: size overflow");
    return (bits + 7U) / 8U;
}

void validateBitIndex(std::size_t index, std::size_t size, const char *owner) {
    if (index >= size)
        throw std::runtime_error(std::string{owner} + ": bit index out of range");
}

class BoundedStorageContext final : public StorageContext {
public:
    BoundedStorageContext(
        const TypeController &types,
        const LayoutResolutionSession &layouts,
        BitAddress base,
        std::size_t typeBitSize
    ) : types_{types}, layouts_{layouts}, base_{base}, typeBitSize_{typeBitSize} {}

    const TypeController &types() const noexcept override { return types_; }

    TypeLayout layoutOf(const TypeId &type) const override {
        return layouts_.layoutOf(type);
    }

    BitValue loadBits(const BitStorage &storage, BitAddress address, std::size_t bitSize) const override {
        validateRange(address, bitSize, "StorageCapability::loadBits");
        return storage.loadBits(address, bitSize);
    }

    void storeBits(BitStorage &storage, BitAddress address, const BitValue &value) const override {
        validateRange(address, value.bitSize(), "StorageCapability::storeBits");
        storage.storeBits(address, value);
    }

private:
    void validateRange(BitAddress address, std::size_t bitSize, const char *owner) const {
        if (address.bitOffset < base_.bitOffset)
            throw std::runtime_error(std::string{owner} + ": access starts before the type storage range");

        const std::size_t relative{address.bitOffset - base_.bitOffset};
        if (relative > typeBitSize_ || bitSize > typeBitSize_ - relative)
            throw std::runtime_error(std::string{owner} + ": access escapes the type storage range");
    }

    const TypeController &types_;
    const LayoutResolutionSession &layouts_;
    BitAddress base_{};
    std::size_t typeBitSize_{0};
};

} // namespace

BitValue::BitValue(std::size_t bitSize) : bitSize_{bitSize}, bytes_(byteCount(bitSize), 0U) {}

BitValue BitValue::fromUnsigned(std::uint64_t value, std::size_t bitSize) {
    if (bitSize > 64U)
        throw std::runtime_error("BitValue::fromUnsigned: width exceeds 64 bits");
    BitValue result{bitSize};
    for (std::size_t index{0}; index < bitSize; ++index)
        result.setBit(index, ((value >> index) & 1U) != 0U);
    return result;
}

bool BitValue::bit(std::size_t index) const {
    validateBitIndex(index, bitSize_, "BitValue::bit");
    return (bytes_[index / 8U] & static_cast<std::uint8_t>(1U << (index % 8U))) != 0U;
}

void BitValue::setBit(std::size_t index, bool value) {
    validateBitIndex(index, bitSize_, "BitValue::setBit");
    const auto mask{static_cast<std::uint8_t>(1U << (index % 8U))};
    auto &byte{bytes_[index / 8U]};
    if (value)
        byte = static_cast<std::uint8_t>(byte | mask);
    else
        byte = static_cast<std::uint8_t>(byte & static_cast<std::uint8_t>(~mask));
}

std::uint64_t BitValue::toUnsigned() const {
    if (bitSize_ > 64U)
        throw std::runtime_error("BitValue::toUnsigned: width exceeds 64 bits");
    std::uint64_t value{0};
    for (std::size_t index{0}; index < bitSize_; ++index) {
        if (bit(index))
            value |= (std::uint64_t{1} << index);
    }
    return value;
}

BitValue BitValue::extract(std::size_t bitOffset, std::size_t bitSize) const {
    if (bitOffset > bitSize_ || bitSize > bitSize_ - bitOffset)
        throw std::runtime_error("BitValue::extract: range out of bounds");
    BitValue result{bitSize};
    for (std::size_t index{0}; index < bitSize; ++index)
        result.setBit(index, bit(bitOffset + index));
    return result;
}

void BitValue::insert(std::size_t bitOffset, const BitValue &value) {
    if (bitOffset > bitSize_ || value.bitSize() > bitSize_ - bitOffset)
        throw std::runtime_error("BitValue::insert: range out of bounds");
    for (std::size_t index{0}; index < value.bitSize(); ++index)
        setBit(bitOffset + index, value.bit(index));
}

BitValue BitValue::shiftedLeft(std::size_t count) const {
    BitValue result{bitSize_};
    if (count >= bitSize_)
        return result;
    for (std::size_t index{0}; index + count < bitSize_; ++index)
        result.setBit(index + count, bit(index));
    return result;
}

BitValue BitValue::shiftedRight(std::size_t count) const {
    BitValue result{bitSize_};
    if (count >= bitSize_)
        return result;
    for (std::size_t index{count}; index < bitSize_; ++index)
        result.setBit(index - count, bit(index));
    return result;
}

BitValue BitValue::masked(const BitValue &mask) const {
    if (mask.bitSize() != bitSize_)
        throw std::runtime_error("BitValue::masked: mask width differs from value width");
    BitValue result{bitSize_};
    for (std::size_t index{0}; index < bitSize_; ++index)
        result.setBit(index, bit(index) && mask.bit(index));
    return result;
}

void BitValue::clearUnusedHighBits() noexcept {
    if (bytes_.empty() || bitSize_ % 8U == 0U)
        return;
    const auto used{static_cast<unsigned>(bitSize_ % 8U)};
    const auto mask{static_cast<std::uint8_t>((1U << used) - 1U)};
    bytes_.back() = static_cast<std::uint8_t>(bytes_.back() & mask);
}

BitStorage::BitStorage(std::size_t bitSize) : bitSize_{bitSize}, bytes_(byteCount(bitSize), 0U) {}

void BitStorage::validateRange(BitAddress address, std::size_t bitSize, const char *owner) const {
    if (address.bitOffset > bitSize_ || bitSize > bitSize_ - address.bitOffset)
        throw std::runtime_error(std::string{owner} + ": bit range out of bounds");
}

BitValue BitStorage::loadBits(BitAddress address, std::size_t bitSize) const {
    validateRange(address, bitSize, "BitStorage::loadBits");
    BitValue result{bitSize};
    for (std::size_t index{0}; index < bitSize; ++index)
        result.setBit(index, bit(address.bitOffset + index));
    return result;
}

void BitStorage::storeBits(BitAddress address, const BitValue &value) {
    validateRange(address, value.bitSize(), "BitStorage::storeBits");
    for (std::size_t index{0}; index < value.bitSize(); ++index) {
        const std::size_t target{address.bitOffset + index};
        const auto mask{static_cast<std::uint8_t>(1U << (target % 8U))};
        auto &byte{bytes_[target / 8U]};
        if (value.bit(index))
            byte = static_cast<std::uint8_t>(byte | mask);
        else
            byte = static_cast<std::uint8_t>(byte & static_cast<std::uint8_t>(~mask));
    }
}

bool BitStorage::bit(std::size_t index) const {
    validateBitIndex(index, bitSize_, "BitStorage::bit");
    return (bytes_[index / 8U] & static_cast<std::uint8_t>(1U << (index % 8U))) != 0U;
}

BitValue StorageController::loadBits(const BitStorage &storage, BitAddress address, std::size_t bitSize) const {
    return storage.loadBits(address, bitSize);
}

void StorageController::storeBits(BitStorage &storage, BitAddress address, const BitValue &value) const {
    storage.storeBits(address, value);
}

BitValue StorageController::load(const TypeId &type, const BitStorage &storage, BitAddress address) const {
    const TypeId canonical{types_.canonical(type)};
    const TypeDefinition &definition{types_.requireType(canonical)};
    auto layoutSession{layouts_.session()};
    const TypeLayout layout{layoutSession.layoutOf(canonical)};
    if (const auto *behavior{definition.capabilities.get<StorageCapability>()}) {
        const BoundedStorageContext context{types_, layoutSession, address, layout.bitSize};
        BitValue value{behavior->load(context, definition, storage, address)};
        if (value.bitSize() != layout.bitSize)
            throw std::runtime_error("StorageController::load: storage capability returned a value with the wrong bit width");
        return value;
    }

    return storage.loadBits(address, layout.bitSize);
}

void StorageController::store(const TypeId &type, BitStorage &storage, BitAddress address, const BitValue &value) const {
    const TypeId canonical{types_.canonical(type)};
    const TypeDefinition &definition{types_.requireType(canonical)};
    auto layoutSession{layouts_.session()};
    const TypeLayout layout{layoutSession.layoutOf(canonical)};
    if (value.bitSize() != layout.bitSize)
        throw std::runtime_error("StorageController::store: value width does not match the type bit width");

    if (const auto *behavior{definition.capabilities.get<StorageCapability>()}) {
        const BoundedStorageContext context{types_, layoutSession, address, layout.bitSize};
        behavior->store(context, definition, storage, address, value);
        return;
    }

    storage.storeBits(address, value);
}

} // namespace novac::assets::types
