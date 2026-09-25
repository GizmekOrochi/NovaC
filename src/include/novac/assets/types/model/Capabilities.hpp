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
    virtual ~CompositionCapability() = default;
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
    virtual ~LayoutContext() = default;
    virtual const TypeController &types() const noexcept = 0;
    virtual TypeLayout layoutOf(const TypeId &type) const = 0;
};

/** Behavior for a type that can produce a bit layout. */
class LayoutCapability : public TypeCapability {
public:
    virtual ~LayoutCapability() = default;
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
    virtual ~StorageContext() = default;
    virtual const TypeController &types() const noexcept = 0;
    virtual TypeLayout layoutOf(const TypeId &type) const = 0;
    virtual BitValue loadBits(const BitStorage &storage, BitAddress address, std::size_t bitSize) const = 0;
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
    virtual ~StorageCapability() = default;
    virtual BitValue load(
        const StorageContext &context,
        const TypeDefinition &type,
        const BitStorage &storage,
        BitAddress address
    ) const = 0;
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
    virtual ~MemberCapability() = default;
    virtual std::optional<TypeMember> findMember(const TypeDefinition &type, std::string_view name) const = 0;
};

} // namespace novac::assets::types
