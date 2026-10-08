#pragma once

#include "novac/assets/types/model/Type.hpp"
#include "novac/assets/types/model/Storage.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace novac::assets::types {

class TypeController;

/** A named or anonymous logical component of a complex type. */
struct TypeComponent {
    std::string name{};
    TypeId type{};
    ExtensionSet extensions{};
};

/** Behavior for types that expose logical child components. */
class CompositionCapability : public TypeCapability {
public:
    /**
     * @brief Destroys the `CompositionCapability` instance.
     */
    virtual ~CompositionCapability() = default;
    /**
     * @brief Returns the value exposed by `components`.
     *
     * @param type Value supplied for `type`.
     * @return Value produced by the operation.
     */
    virtual std::span<const TypeComponent> components(const TypeDefinition &type) const = 0;
};

/** Layout of one logical component, expressed in bits. */
struct ComponentLayout {
    std::size_t componentIndex{0};
    std::size_t bitOffset{0};
    std::size_t bitSize{0};
    std::size_t alignmentBits{1};

    /** Byte offset rounded down. Meaningful only for byte-aligned components. */
    std::size_t byteOffset() const noexcept { return bitOffset / 8U; }
    /** Storage size rounded up to full bytes. */
    std::size_t sizeBytes() const noexcept { return (bitSize + 7U) / 8U; }
    /** Alignment rounded up to full bytes. */
    std::size_t alignmentBytes() const noexcept { return (alignmentBits + 7U) / 8U; }
    /** True when this component starts on a byte boundary. */
    bool byteAligned() const noexcept { return bitOffset % 8U == 0U; }
};

/** Layout produced by the active layout semantics for a type. */
struct TypeLayout {
    std::size_t bitSize{0};
    std::size_t alignmentBits{1};
    std::vector<ComponentLayout> components{};
    ExtensionSet extensions{};

    /** Storage size rounded up to full bytes. */
    std::size_t sizeBytes() const noexcept { return (bitSize + 7U) / 8U; }
    /** Alignment rounded up to full bytes. */
    std::size_t alignmentBytes() const noexcept { return (alignmentBits + 7U) / 8U; }
    /** True when both size and alignment are byte-addressable. */
    bool byteAddressable() const noexcept { return bitSize % 8U == 0U && alignmentBits % 8U == 0U; }
};

/** Recursive layout context passed to custom layout behaviors. */
class LayoutContext {
public:
    /**
     * @brief Destroys the `LayoutContext` instance.
     */
    virtual ~LayoutContext() = default;
    /**
     * @brief Returns the value exposed by `types`.
     *
     * @return Value produced by the operation.
     */
    virtual const TypeController &types() const noexcept = 0;
    /**
     * @brief Performs the `layoutOf` operation.
     *
     * @param type Value supplied for `type`.
     * @return Value produced by the operation.
     */
    virtual TypeLayout layoutOf(const TypeId &type) const = 0;
};

/** Behavior for a type that can produce a bit layout. */
class LayoutCapability : public TypeCapability {
public:
    /**
     * @brief Destroys the `LayoutCapability` instance.
     */
    virtual ~LayoutCapability() = default;
    /**
     * @brief Computes the result of `compute`.
     *
     * @param context Value supplied for `context`.
     * @param type Value supplied for `type`.
     * @return Value produced by the operation.
     */
    virtual TypeLayout compute(const LayoutContext &context, const TypeDefinition &type) const = 0;
};


/**
 * @brief Low-level storage context exposed to custom type storage behavior.
 *
 * The context represents one bounded storage operation. Raw loadBits/storeBits
 * requests made by a StorageCapability must stay inside the TypeLayout
 * range of the value currently being loaded or stored. layoutOf() participates
 * in the same layout-resolution session, so repeated nested layout queries reuse
 * one recursion guard and cache.
 */
class StorageContext {
public:
    /**
     * @brief Destroys the `StorageContext` instance.
     */
    virtual ~StorageContext() = default;
    /**
     * @brief Returns the value exposed by `types`.
     *
     * @return Value produced by the operation.
     */
    virtual const TypeController &types() const noexcept = 0;
    /**
     * @brief Performs the `layoutOf` operation.
     *
     * @param type Value supplied for `type`.
     * @return Value produced by the operation.
     */
    virtual TypeLayout layoutOf(const TypeId &type) const = 0;
    /**
     * @brief Loads data through `loadBits`.
     *
     * @param storage Value supplied for `storage`.
     * @param address Value supplied for `address`.
     * @param bitSize Value supplied for `bitSize`.
     * @return Value produced by the operation.
     */
    virtual BitValue loadBits(const BitStorage &storage, BitAddress address, std::size_t bitSize) const = 0;
    /**
     * @brief Stores data through `storeBits`.
     *
     * @param storage Value supplied for `storage`.
     * @param address Value supplied for `address`.
     * @param value Value supplied for `value`.
     */
    virtual void storeBits(BitStorage &storage, BitAddress address, const BitValue &value) const = 0;
};

/**
 * @brief Customizes how a type's exact bits are loaded from and stored to bit storage.
 *
 * A type has one bit width: the bits declared by the type are the real bits it
 * occupies. StorageCapability may transform or validate those bits while loading
 * and storing them, but it does not introduce a second bit width.
 * Raw storage access goes through StorageContext and is bounded to the TypeLayout
 * range assigned to the current value.
 */
class StorageCapability : public TypeCapability {
public:
    /**
     * @brief Destroys the `StorageCapability` instance.
     */
    virtual ~StorageCapability() = default;
    /**
     * @brief Loads data through `load`.
     *
     * @param context Value supplied for `context`.
     * @param type Value supplied for `type`.
     * @param storage Value supplied for `storage`.
     * @param address Value supplied for `address`.
     * @return Value produced by the operation.
     */
    virtual BitValue load(
        const StorageContext &context,
        const TypeDefinition &type,
        const BitStorage &storage,
        BitAddress address
    ) const = 0;
    /**
     * @brief Stores data through `store`.
     *
     * @param context Value supplied for `context`.
     * @param type Value supplied for `type`.
     * @param storage Value supplied for `storage`.
     * @param address Value supplied for `address`.
     * @param value Value supplied for `value`.
     */
    virtual void store(
        const StorageContext &context,
        const TypeDefinition &type,
        BitStorage &storage,
        BitAddress address,
        const BitValue &value
    ) const = 0;
};

/**
 * @brief Non-owning semantic member view exposed by a type.
 *
 * Members need not correspond to storage fields. The component pointer remains
 * valid only while the owning TypeDefinition remains alive and unchanged. In
 * normal finalized type configurations this means the view may be used for the
 * lifetime of the owning TypeController.
 */
struct TypeMember {
    const TypeComponent *component{nullptr};
    std::size_t componentIndex{0};

    /** Member name from the underlying component. */
    std::string_view name() const noexcept { return component ? std::string_view{component->name} : std::string_view{}; }
    /** Member type from the underlying component. */
    const TypeId &type() const {
        if (!component)
            throw std::runtime_error("TypeMember::type: member has no component");
        return component->type;
    }
    /** Typed metadata attached to the underlying component. */
    const ExtensionSet &extensions() const {
        if (!component)
            throw std::runtime_error("TypeMember::extensions: member has no component");
        return component->extensions;
    }
};

/** Behavior for named member lookup. */
class MemberCapability : public TypeCapability {
public:
    /**
     * @brief Destroys the `MemberCapability` instance.
     */
    virtual ~MemberCapability() = default;
    /**
     * @brief Finds the value requested by `findMember`.
     *
     * @param type Value supplied for `type`.
     * @param name Value supplied for `name`.
     * @return Value produced by the operation.
     */
    virtual std::optional<TypeMember> findMember(const TypeDefinition &type, std::string_view name) const = 0;
};

} // namespace novac::assets::types
