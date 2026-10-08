#include "novac/assets/memory/model/Memory.hpp"

#include <limits>
#include <utility>

namespace novac::assets::memory {

/**
 * @brief Constructs a `MemoryError` instance.
 *
 * @param code Value supplied for `code`.
 * @param message Value supplied for `message`.
 */
MemoryError::MemoryError(MemoryErrorCode code, std::string message)
    : std::runtime_error{std::move(message)}, code_{code} {}

/**
 * @brief Implements the `code` operation.
 *
 * @return Value produced by the operation.
 */
MemoryErrorCode MemoryError::code() const noexcept { return code_; }

/**
 * @brief Implements the `valid` operation.
 *
 * @return Value produced by the operation.
 */
bool AddressSpaceId::valid() const noexcept { return value != 0; }
/**
 * @brief Implements the `valid` operation.
 *
 * @return Value produced by the operation.
 */
bool RegionId::valid() const noexcept { return value != 0; }
/**
 * @brief Implements the `valid` operation.
 *
 * @return Value produced by the operation.
 */
bool AllocationId::valid() const noexcept { return value != 0; }
/**
 * @brief Implements the `valid` operation.
 *
 * @return Value produced by the operation.
 */
bool LifetimeId::valid() const noexcept { return value != 0; }

/**
 * @brief Implements the `operator bool` operation.
 *
 * @return Value produced by the operation.
 */
AddressSpaceId::operator bool() const noexcept { return valid(); }
/**
 * @brief Implements the `operator bool` operation.
 *
 * @return Value produced by the operation.
 */
RegionId::operator bool() const noexcept { return valid(); }
/**
 * @brief Implements the `operator bool` operation.
 *
 * @return Value produced by the operation.
 */
AllocationId::operator bool() const noexcept { return valid(); }
/**
 * @brief Implements the `operator bool` operation.
 *
 * @return Value produced by the operation.
 */
LifetimeId::operator bool() const noexcept { return valid(); }

/**
 * @brief Invokes the callable object.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
std::size_t AddressSpaceIdHash::operator()(AddressSpaceId id) const noexcept {
    return std::hash<std::size_t>{}(id.value);
}
/**
 * @brief Invokes the callable object.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
std::size_t RegionIdHash::operator()(RegionId id) const noexcept {
    return std::hash<std::size_t>{}(id.value);
}
/**
 * @brief Invokes the callable object.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
std::size_t AllocationIdHash::operator()(AllocationId id) const noexcept {
    return std::hash<std::size_t>{}(id.value);
}
/**
 * @brief Invokes the callable object.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
std::size_t LifetimeIdHash::operator()(LifetimeId id) const noexcept {
    return std::hash<std::size_t>{}(id.value);
}

/**
 * @brief Implements the `advanced` operation.
 *
 * @param bits Value supplied for `bits`.
 * @return Value produced by the operation.
 */
Address Address::advanced(std::size_t bits) const {
    if (bitOffset > std::numeric_limits<std::size_t>::max() - bits)
        throw MemoryError{MemoryErrorCode::OutOfBounds, "Address::advanced: bit address overflow"};
    return Address{space, bitOffset + bits};
}

/**
 * @brief Checks the condition represented by `empty`.
 *
 * @return Value produced by the operation.
 */
bool AddressRange::empty() const noexcept { return bitSize == 0; }

/**
 * @brief Completes the operation represented by `endBitOffset`.
 *
 * @return Value produced by the operation.
 */
std::size_t AddressRange::endBitOffset() const {
    if (begin.bitOffset > std::numeric_limits<std::size_t>::max() - bitSize)
        throw MemoryError{MemoryErrorCode::OutOfBounds, "AddressRange: bit range overflow"};
    return begin.bitOffset + bitSize;
}

/**
 * @brief Checks the condition represented by `contains`.
 *
 * @param address Value supplied for `address`.
 * @return Value produced by the operation.
 */
bool AddressRange::contains(Address address) const {
    if (!(address.space == begin.space))
        return false;
    const std::size_t end{endBitOffset()};
    return address.bitOffset >= begin.bitOffset && address.bitOffset < end;
}

/**
 * @brief Checks the condition represented by `contains`.
 *
 * @param other Value supplied for `other`.
 * @return Value produced by the operation.
 */
bool AddressRange::contains(const AddressRange &other) const {
    if (!(other.begin.space == begin.space))
        return false;
    return other.begin.bitOffset >= begin.bitOffset && other.endBitOffset() <= endBitOffset();
}

/**
 * @brief Checks the condition represented by `overlaps`.
 *
 * @param other Value supplied for `other`.
 * @return Value produced by the operation.
 */
bool AddressRange::overlaps(const AddressRange &other) const {
    if (!(other.begin.space == begin.space))
        return false;
    return begin.bitOffset < other.endBitOffset() && other.begin.bitOffset < endBitOffset();
}

/**
 * @brief Checks the condition represented by `active`.
 *
 * @return Value produced by the operation.
 */
bool Allocation::active() const noexcept { return state == AllocationState::Active; }

} // namespace novac::assets::memory
