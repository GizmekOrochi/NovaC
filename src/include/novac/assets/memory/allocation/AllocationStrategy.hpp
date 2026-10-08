#pragma once

#include "novac/assets/memory/model/Memory.hpp"

#include <cstddef>
#include <optional>
#include <span>

namespace novac::assets::memory {

struct AllocationRequest {
    AddressRange region{};
    std::size_t bitSize{0};
    std::size_t alignmentBits{1};
};

struct AllocationPlacement {
    std::size_t regionRelativeBitOffset{0};
};

class AllocationStrategy {
public:
    /**
     * @brief Destroys the `AllocationStrategy` instance.
     */
    virtual ~AllocationStrategy() = default;
    /**
     * @brief Performs the `place` operation.
     *
     * @param request Value supplied for `request`.
     * @param existing Value supplied for `existing`.
     * @return Value produced by the operation.
     */
    virtual std::optional<AllocationPlacement> place(
        const AllocationRequest &request,
        std::span<const Allocation> existing
    ) = 0;
};

/** Default first-fit strategy using active allocations as occupied ranges. */
class LinearAllocationStrategy final : public AllocationStrategy {
public:
    /**
     * @brief Performs the `place` operation.
     *
     * @param request Value supplied for `request`.
     * @param existing Value supplied for `existing`.
     * @return Value produced by the operation.
     */
    std::optional<AllocationPlacement> place(
        const AllocationRequest &request,
        std::span<const Allocation> existing
    ) override;

private:
    /**
     * @brief Performs the `alignUp` operation.
     *
     * @param value Value supplied for `value`.
     * @param alignment Value supplied for `alignment`.
     * @return Value produced by the operation.
     */
    static std::size_t alignUp(std::size_t value, std::size_t alignment);
    /**
     * @brief Performs the `fits` operation.
     *
     * @param begin Value supplied for `begin`.
     * @param size Value supplied for `size`.
     * @param end Value supplied for `end`.
     * @return Value produced by the operation.
     */
    static bool fits(std::size_t begin, std::size_t size, std::size_t end) noexcept;
};

} // namespace novac::assets::memory
