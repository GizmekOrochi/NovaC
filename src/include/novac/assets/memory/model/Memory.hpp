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
    /**
     * @brief Constructs a `MemoryError` instance.
     *
     * @param code Value supplied for `code`.
     * @param message Value supplied for `message`.
     */
    MemoryError(MemoryErrorCode code, std::string message);
    /**
     * @brief Performs the `code` operation.
     *
     * @return Value produced by the operation.
     */
    MemoryErrorCode code() const noexcept;

private:
    MemoryErrorCode code_;
};

struct AddressSpaceId {
    std::size_t value{0};
    /**
     * @brief Performs the `valid` operation.
     *
     * @return Value produced by the operation.
     */
    bool valid() const noexcept;
    /**
     * @brief Implements the `operator bool` operation.
     *
     * @return Value produced by the operation.
     */
    explicit operator bool() const noexcept;
    friend bool operator==(const AddressSpaceId &, const AddressSpaceId &) = default;
};

struct RegionId {
    std::size_t value{0};
    /**
     * @brief Performs the `valid` operation.
     *
     * @return Value produced by the operation.
     */
    bool valid() const noexcept;
    /**
     * @brief Implements the `operator bool` operation.
     *
     * @return Value produced by the operation.
     */
    explicit operator bool() const noexcept;
    friend bool operator==(const RegionId &, const RegionId &) = default;
};

struct AllocationId {
    std::size_t value{0};
    /**
     * @brief Performs the `valid` operation.
     *
     * @return Value produced by the operation.
     */
    bool valid() const noexcept;
    /**
     * @brief Implements the `operator bool` operation.
     *
     * @return Value produced by the operation.
     */
    explicit operator bool() const noexcept;
    friend bool operator==(const AllocationId &, const AllocationId &) = default;
};

struct LifetimeId {
    std::size_t value{0};
    /**
     * @brief Performs the `valid` operation.
     *
     * @return Value produced by the operation.
     */
    bool valid() const noexcept;
    /**
     * @brief Implements the `operator bool` operation.
     *
     * @return Value produced by the operation.
     */
    explicit operator bool() const noexcept;
    friend bool operator==(const LifetimeId &, const LifetimeId &) = default;
};

struct AddressSpaceIdHash {
    /**
     * @brief Invokes the callable object.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    std::size_t operator()(AddressSpaceId id) const noexcept;
};
struct RegionIdHash {
    /**
     * @brief Invokes the callable object.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    std::size_t operator()(RegionId id) const noexcept;
};
struct AllocationIdHash {
    /**
     * @brief Invokes the callable object.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    std::size_t operator()(AllocationId id) const noexcept;
};
struct LifetimeIdHash {
    /**
     * @brief Invokes the callable object.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    std::size_t operator()(LifetimeId id) const noexcept;
};

/** Bit-precise address within one NovaC address space. */
struct Address {
    AddressSpaceId space{};
    std::size_t bitOffset{0};

    /**
     * @brief Performs the `advanced` operation.
     *
     * @param bits Value supplied for `bits`.
     * @return Value produced by the operation.
     */
    Address advanced(std::size_t bits) const;
    friend bool operator==(const Address &, const Address &) = default;
};

/** Continuous bit range constrained to one address space. */
struct AddressRange {
    Address begin{};
    std::size_t bitSize{0};

    /**
     * @brief Checks the condition represented by `empty`.
     *
     * @return Value produced by the operation.
     */
    bool empty() const noexcept;
    /**
     * @brief Completes the operation represented by `endBitOffset`.
     *
     * @return Value produced by the operation.
     */
    std::size_t endBitOffset() const;
    /**
     * @brief Checks the condition represented by `contains`.
     *
     * @param address Value supplied for `address`.
     * @return Value produced by the operation.
     */
    bool contains(Address address) const;
    /**
     * @brief Checks the condition represented by `contains`.
     *
     * @param other Value supplied for `other`.
     * @return Value produced by the operation.
     */
    bool contains(const AddressRange &other) const;
    /**
     * @brief Checks the condition represented by `overlaps`.
     *
     * @param other Value supplied for `other`.
     * @return Value produced by the operation.
     */
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

    /**
     * @brief Checks the condition represented by `active`.
     *
     * @return Value produced by the operation.
     */
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
