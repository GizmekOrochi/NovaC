#include "novac/assets/memory/allocation/AllocationStrategy.hpp"

#include <algorithm>
#include <limits>
#include <vector>

namespace novac::assets::memory {

/**
 * @brief Implements the `place` operation.
 *
 * @param request Value supplied for `request`.
 * @param existing Value supplied for `existing`.
 * @return Value produced by the operation.
 */
std::optional<AllocationPlacement> LinearAllocationStrategy::place(const AllocationRequest &request, std::span<const Allocation> existing) {
    if (request.bitSize == 0 || request.alignmentBits == 0)
        return std::nullopt;

    std::vector<AddressRange> occupied;
    occupied.reserve(existing.size());
    for (const Allocation &allocation : existing) {
        if (allocation.active() && allocation.range.begin.space == request.region.begin.space)
            occupied.push_back(allocation.range);
    }
    std::sort(occupied.begin(), occupied.end(), [](const AddressRange &left, const AddressRange &right) {
        return left.begin.bitOffset < right.begin.bitOffset;
    });

    const std::size_t regionEnd{request.region.endBitOffset()};
    std::size_t candidate{alignUp(request.region.begin.bitOffset, request.alignmentBits)};

    for (const AddressRange &range : occupied) {
        const std::size_t rangeEnd{range.endBitOffset()};
        if (rangeEnd <= candidate)
            continue;

        if (range.begin.bitOffset > candidate 
            && fits(candidate, request.bitSize, range.begin.bitOffset) 
            && request.region.contains(AddressRange{Address{request.region.begin.space, candidate}, request.bitSize})) {
            return AllocationPlacement{candidate - request.region.begin.bitOffset};
        }

        if (range.overlaps(request.region) || request.region.contains(range) || range.contains(request.region.begin)) {
            candidate = alignUp(std::max(candidate, rangeEnd), request.alignmentBits);
            if (candidate > regionEnd)
                return std::nullopt;
        }
    }

    if (!fits(candidate, request.bitSize, regionEnd))
        return std::nullopt;
    if (!request.region.contains(AddressRange{Address{request.region.begin.space, candidate}, request.bitSize}))
        return std::nullopt;
    return AllocationPlacement{candidate - request.region.begin.bitOffset};
}

/**
 * @brief Implements the `alignUp` operation.
 *
 * @param value Value supplied for `value`.
 * @param alignment Value supplied for `alignment`.
 * @return Value produced by the operation.
 */
std::size_t LinearAllocationStrategy::alignUp(std::size_t value, std::size_t alignment) {
    if (alignment == 0)
        return value;
    const std::size_t remainder{value % alignment};
    if (remainder == 0)
        return value;
    const std::size_t padding{alignment - remainder};
    if (value > std::numeric_limits<std::size_t>::max() - padding)
        throw MemoryError{MemoryErrorCode::OutOfBounds, "LinearAllocationStrategy: alignment overflow"};
    return value + padding;
}

/**
 * @brief Implements the `fits` operation.
 *
 * @param begin Value supplied for `begin`.
 * @param size Value supplied for `size`.
 * @param end Value supplied for `end`.
 * @return Value produced by the operation.
 */
bool LinearAllocationStrategy::fits(std::size_t begin, std::size_t size, std::size_t end) noexcept {
    return begin <= end && size <= end - begin;
}

} // namespace novac::assets::memory
