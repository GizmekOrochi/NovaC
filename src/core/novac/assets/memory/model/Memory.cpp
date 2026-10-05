#include "novac/assets/memory/model/Memory.hpp"

#include <limits>
#include <utility>

namespace novac::assets::memory {

MemoryError::MemoryError(MemoryErrorCode code, std::string message)
    : std::runtime_error{std::move(message)}, code_{code} {}

MemoryErrorCode MemoryError::code() const noexcept { return code_; }

bool AddressSpaceId::valid() const noexcept { return value != 0; }
bool RegionId::valid() const noexcept { return value != 0; }
bool AllocationId::valid() const noexcept { return value != 0; }
bool LifetimeId::valid() const noexcept { return value != 0; }

AddressSpaceId::operator bool() const noexcept { return valid(); }
RegionId::operator bool() const noexcept { return valid(); }
AllocationId::operator bool() const noexcept { return valid(); }
LifetimeId::operator bool() const noexcept { return valid(); }

std::size_t AddressSpaceIdHash::operator()(AddressSpaceId id) const noexcept {
    return std::hash<std::size_t>{}(id.value);
}
std::size_t RegionIdHash::operator()(RegionId id) const noexcept {
    return std::hash<std::size_t>{}(id.value);
}
std::size_t AllocationIdHash::operator()(AllocationId id) const noexcept {
    return std::hash<std::size_t>{}(id.value);
}
std::size_t LifetimeIdHash::operator()(LifetimeId id) const noexcept {
    return std::hash<std::size_t>{}(id.value);
}

Address Address::advanced(std::size_t bits) const {
    if (bitOffset > std::numeric_limits<std::size_t>::max() - bits)
        throw MemoryError{MemoryErrorCode::OutOfBounds, "Address::advanced: bit address overflow"};
    return Address{space, bitOffset + bits};
}

bool AddressRange::empty() const noexcept { return bitSize == 0; }

std::size_t AddressRange::endBitOffset() const {
    if (begin.bitOffset > std::numeric_limits<std::size_t>::max() - bitSize)
        throw MemoryError{MemoryErrorCode::OutOfBounds, "AddressRange: bit range overflow"};
    return begin.bitOffset + bitSize;
}

bool AddressRange::contains(Address address) const {
    if (!(address.space == begin.space))
        return false;
    const std::size_t end{endBitOffset()};
    return address.bitOffset >= begin.bitOffset && address.bitOffset < end;
}

bool AddressRange::contains(const AddressRange &other) const {
    if (!(other.begin.space == begin.space))
        return false;
    return other.begin.bitOffset >= begin.bitOffset && other.endBitOffset() <= endBitOffset();
}

bool AddressRange::overlaps(const AddressRange &other) const {
    if (!(other.begin.space == begin.space))
        return false;
    return begin.bitOffset < other.endBitOffset() && other.begin.bitOffset < endBitOffset();
}

bool Allocation::active() const noexcept { return state == AllocationState::Active; }

} // namespace novac::assets::memory
