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
    virtual ~AllocationStrategy() = default;
    virtual std::optional<AllocationPlacement> place(
        const AllocationRequest &request,
        std::span<const Allocation> existing
    ) = 0;
};

/** Default first-fit strategy using active allocations as occupied ranges. */
class LinearAllocationStrategy final : public AllocationStrategy {
public:
    std::optional<AllocationPlacement> place(
        const AllocationRequest &request,
        std::span<const Allocation> existing
    ) override;

private:
    static std::size_t alignUp(std::size_t value, std::size_t alignment);
    static bool fits(std::size_t begin, std::size_t size, std::size_t end) noexcept;
};

} // namespace novac::assets::memory
