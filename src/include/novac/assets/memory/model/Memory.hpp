#pragma once

#include "novac/assets/memory/behavior/MemoryCapabilities.hpp"
#include "novac/assets/types/model/Type.hpp"

#include <cstddef>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>

namespace novac::assets::memory {

/** Error categories produced by the low-level memory model. */
enum class MemoryErrorCode {
    InvalidArgument,
    UnknownAddressSpace,
    UnknownRegion,
    UnknownAllocation,
    UnknownLifetime,
    OutOfBounds,
    InvalidAlignment,
    OutOfMemory,
    ReleasedAllocation,
    DeadLifetime,
    InvalidReference,
    DuplicateName,
    BackendMismatch,
    UnsupportedOperation
};

/** Exception carrying a stable machine-readable memory error category. */
class MemoryError : public std::runtime_error {
public:
    MemoryError(MemoryErrorCode code, std::string message);
    MemoryErrorCode code() const noexcept;

private:
    MemoryErrorCode code_;
};

struct AddressSpaceId {
    std::size_t value{0};
    bool valid() const noexcept;
    explicit operator bool() const noexcept;
    friend bool operator==(const AddressSpaceId &, const AddressSpaceId &) = default;
};

struct RegionId {
    std::size_t value{0};
    bool valid() const noexcept;
    explicit operator bool() const noexcept;
    friend bool operator==(const RegionId &, const RegionId &) = default;
};

struct AllocationId {
    std::size_t value{0};
    bool valid() const noexcept;
    explicit operator bool() const noexcept;
    friend bool operator==(const AllocationId &, const AllocationId &) = default;
};

struct LifetimeId {
    std::size_t value{0};
    bool valid() const noexcept;
    explicit operator bool() const noexcept;
    friend bool operator==(const LifetimeId &, const LifetimeId &) = default;
};

struct AddressSpaceIdHash {
    std::size_t operator()(AddressSpaceId id) const noexcept;
};
struct RegionIdHash {
    std::size_t operator()(RegionId id) const noexcept;
};
struct AllocationIdHash {
    std::size_t operator()(AllocationId id) const noexcept;
};
struct LifetimeIdHash {
    std::size_t operator()(LifetimeId id) const noexcept;
};

/** Bit-precise address within one NovaC address space. */
struct Address {
    AddressSpaceId space{};
    std::size_t bitOffset{0};

    Address advanced(std::size_t bits) const;
    friend bool operator==(const Address &, const Address &) = default;
};

/** Continuous bit range constrained to one address space. */
struct AddressRange {
    Address begin{};
    std::size_t bitSize{0};

    bool empty() const noexcept;
    std::size_t endBitOffset() const;
    bool contains(Address address) const;
    bool contains(const AddressRange &other) const;
    bool overlaps(const AddressRange &other) const;
};

/** Immutable description of one addressable memory space. */
struct AddressSpaceDefinition {
    AddressSpaceId id{};
    std::string name{};
    std::size_t bitSize{0};
    types::ExtensionSet extensions{};
    MemoryCapabilitySet capabilities{};
};

/** Named view over a bit range. Regions do not own storage or impose hierarchy. */
struct MemoryRegion {
    RegionId id{};
    std::string name{};
    AddressRange range{};
    types::ExtensionSet extensions{};
    MemoryCapabilitySet capabilities{};
};

enum class AllocationState {
    Active,
    Released
};

/** Reserved raw bit range. Allocations intentionally carry no TypeId. */
struct Allocation {
    AllocationId id{};
    RegionId region{};
    AddressRange range{};
    std::size_t alignmentBits{1};
    std::optional<LifetimeId> lifetime{};
    AllocationState state{AllocationState::Active};
    types::ExtensionSet extensions{};
    MemoryCapabilitySet capabilities{};

    bool active() const noexcept;
};

struct Lifetime {
    LifetimeId id{};
    bool alive{true};
    types::ExtensionSet extensions{};
};

struct MemoryReference {
    Address address{};
    types::TypeId type{};
    std::optional<AllocationId> provenance{};
};

/** Target scope used by generic memory-operation dispatch. */
using MemoryTarget = std::variant<AddressSpaceId, RegionId, AllocationId>;

} // namespace novac::assets::memory
