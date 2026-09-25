#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace novac::assets::types {

/**
 * @brief Source-level name used to identify a type in the language type system.
 */
struct TypeId {
    /** @brief Source-level type name. */
    std::string name{};

    /** @brief Constructs an invalid/empty type identifier. */
    TypeId() = default;

    /**
     * @brief Constructs a type identifier from its source-level name.
     * @param value Type name.
     */
    explicit TypeId(std::string value) : name{std::move(value)} {}

    /**
     * @brief Reports whether the identifier contains a non-empty name.
     * @return true when the identifier is valid.
     */
    bool valid() const noexcept { return !name.empty(); }

    /**
     * @brief Boolean shorthand for valid().
     */
    explicit operator bool() const noexcept { return valid(); }

    /** @brief Compares two type identifiers by name. */
    friend bool operator==(const TypeId &, const TypeId &) = default;
};

/**
 * @brief Hash functor for TypeId.
 */
struct TypeIdHash {
    /**
     * @brief Computes a hash from the contained type name.
     * @param id Type identifier to hash.
     * @return Hash value suitable for unordered containers.
     */
    std::size_t operator()(const TypeId &id) const noexcept {
        return std::hash<std::string>{}(id.name);
    }
};

/**
 * @brief Type-indexed metadata storage used by the type system.
 *
 * Extensions allow language implementations to attach custom strongly typed
 * metadata without extending NovaC's built-in descriptors. Once frozen, the
 * set rejects mutation and all stored objects remain const.
 */
class ExtensionSet final {
public:
    /**
     * @brief Constructs and stores an extension of type T.
     * @tparam T Extension type.
     * @tparam Args Constructor argument types.
     * @param args Arguments forwarded to T's constructor.
     * @return Const reference to the stored extension.
     * @throws std::runtime_error if the set has already been frozen.
     */
    template <typename T, typename... Args>
    const T &emplace(Args &&...args) {
        static_assert(!std::is_reference_v<T>, "extension type cannot be a reference");
        ensureMutable();
        auto value{std::make_shared<const T>(std::forward<Args>(args)...)};
        const T &reference{*value};
        values_.insert_or_assign(std::type_index(typeid(T)), std::move(value));
        return reference;
    }

    /**
     * @brief Stores an extension value.
     * @tparam T Extension type.
     * @param value Value to store.
     * @return Const reference to the stored extension.
     * @throws std::runtime_error if the set has already been frozen.
     */
    template <typename T>
    const T &set(T value) {
        return emplace<T>(std::move(value));
    }

    /**
     * @brief Checks whether an extension of type T is present.
     * @tparam T Extension type.
     * @return true when an extension of type T exists.
     */
    template <typename T>
    bool has() const noexcept {
        return values_.find(std::type_index(typeid(T))) != values_.end();
    }

    /**
     * @brief Retrieves an extension by its C++ type.
     * @tparam T Extension type.
     * @return Pointer to the stored extension, or nullptr when absent.
     */
    template <typename T>
    const T *get() const noexcept {
        const auto it{values_.find(std::type_index(typeid(T)))};
        return it == values_.end() ? nullptr : static_cast<const T *>(it->second.get());
    }

    /**
     * @brief Removes an extension by type.
     * @tparam T Extension type.
     * @throws std::runtime_error if the set has already been frozen.
     */
    template <typename T>
    void erase() {
        ensureMutable();
        values_.erase(std::type_index(typeid(T)));
    }

    /**
     * @brief Removes all stored extensions.
     * @throws std::runtime_error if the set has already been frozen.
     */
    void clear() {
        ensureMutable();
        values_.clear();
    }

    /** @brief Prevents any further mutation of the extension set. */
    void freeze() noexcept { frozen_ = true; }

    /** @brief Reports whether the set is immutable. */
    bool frozen() const noexcept { return frozen_; }

    /** @brief Returns the number of stored extension types. */
    std::size_t size() const noexcept { return values_.size(); }

    /** @brief Reports whether no extensions are stored. */
    bool empty() const noexcept { return values_.empty(); }

private:
    void ensureMutable() const {
        if (frozen_) {
            throw std::runtime_error("ExtensionSet: committed metadata is immutable");
        }
    }

    std::unordered_map<std::type_index, std::shared_ptr<const void>> values_{};
    bool frozen_{false};
};

/**
 * @brief Base class for optional behavior attached to a type descriptor.
 *
 * Capabilities complement ExtensionSet: extensions store immutable metadata,
 * while capabilities expose polymorphic behavior such as composition, layout,
 * or member lookup.
 */
class TypeCapability {
public:
    virtual ~TypeCapability() = default;
};

/**
 * @brief Type-indexed registry of behavior capabilities.
 *
 * Capabilities are registered by their public interface type. Exactly one
 * active implementation is stored per capability interface; registering the
 * same interface again replaces the previous behavior. Target- or ABI-specific
 * variants should therefore be selected by the capability/context rather than
 * stored as parallel entries under the same interface.
 */
class TypeCapabilities final {
public:
    template <typename Capability, typename Implementation = Capability, typename... Args>
    const Implementation &emplace(Args &&...args) {
        static_assert(std::is_base_of_v<TypeCapability, Capability>, "Capability must derive from TypeCapability");
        static_assert(std::is_base_of_v<Capability, Implementation>, "Implementation must derive from Capability");
        ensureMutable();
        auto value{std::make_shared<const Implementation>(std::forward<Args>(args)...)};
        const Implementation &reference{*value};
        values_.insert_or_assign(std::type_index(typeid(Capability)), std::move(value));
        return reference;
    }

    template <typename Capability>
    bool has() const noexcept {
        static_assert(std::is_base_of_v<TypeCapability, Capability>, "Capability must derive from TypeCapability");
        return values_.find(std::type_index(typeid(Capability))) != values_.end();
    }

    template <typename Capability>
    const Capability *get() const noexcept {
        static_assert(std::is_base_of_v<TypeCapability, Capability>, "Capability must derive from TypeCapability");
        const auto it{values_.find(std::type_index(typeid(Capability)))};
        return it == values_.end() ? nullptr : static_cast<const Capability *>(it->second.get());
    }

    template <typename Capability>
    void erase() {
        static_assert(std::is_base_of_v<TypeCapability, Capability>, "Capability must derive from TypeCapability");
        ensureMutable();
        values_.erase(std::type_index(typeid(Capability)));
    }

    void clear() {
        ensureMutable();
        values_.clear();
    }

    void freeze() noexcept { frozen_ = true; }
    bool frozen() const noexcept { return frozen_; }
    std::size_t size() const noexcept { return values_.size(); }
    bool empty() const noexcept { return values_.empty(); }

private:
    void ensureMutable() const {
        if (frozen_)
            throw std::runtime_error("TypeCapabilities: committed behavior is immutable");
    }

    std::unordered_map<std::type_index, std::shared_ptr<const TypeCapability>> values_{};
    bool frozen_{false};
};

/**
 * @brief Base descriptor stored by TypeController.
 *
 * Custom type kinds can derive from this class, attach strongly typed metadata
 * through extensions, and expose optional behavior through capabilities.
 */
class TypeDefinition {
public:
    /**
     * @brief Constructs a type descriptor.
     * @param typeId Identifier owned by the descriptor.
     */
    explicit TypeDefinition(TypeId typeId) : id{std::move(typeId)} {}

    /** @brief Virtual destructor for custom type descriptors. */
    virtual ~TypeDefinition() = default;

    /** @brief Identifier of this type. */
    TypeId id{};
    /** @brief Language-defined metadata attached to the type. */
    ExtensionSet extensions{};
    /** @brief Optional polymorphic behavior exposed by the type. */
    TypeCapabilities capabilities{};

    /**
     * @brief Finalizes the descriptor and freezes metadata and capabilities.
     *
     * This operation is owned by the framework. Derived descriptors may finish
     * their own state in onFreeze() before the public extension points become
     * immutable. Calling freeze() more than once is harmless.
     */
    void freeze() {
        if (frozen_)
            return;
        onFreeze();
        extensions.freeze();
        capabilities.freeze();
        frozen_ = true;
    }

    /** @brief Reports whether the descriptor has been finalized. */
    bool frozen() const noexcept { return frozen_; }

protected:
    /**
     * @brief Hook invoked once by freeze() before extension points are frozen.
     */
    virtual void onFreeze() {}

private:
    bool frozen_{false};
};

/**
 * @brief Signedness category of a primitive type.
 */
enum class PrimitiveSignedness {
    /** Signedness does not apply to the primitive. */
    NotApplicable,
    /** Signed numeric primitive. */
    Signed,
    /** Unsigned numeric primitive. */
    Unsigned
};

/**
 * @brief Byte order used by a primitive representation.
 */
enum class Endianness {
    /** Target-native byte order. */
    Native,
    /** Little-endian byte order. */
    Little,
    /** Big-endian byte order. */
    Big
};

/**
 * @brief Conventional integer value encodings.
 */
enum class IntegerEncoding {
    PureBinary,
    TwosComplement,
    OnesComplement,
    SignMagnitude
};

/**
 * @brief Base class for optional primitive representation descriptors.
 *
 * Languages may derive their own representation descriptor when the built-in
 * convenience representations do not describe the target format.
 */
class PrimitiveRepresentation {
public:
    /** @brief Virtual destructor for custom representation descriptors. */
    virtual ~PrimitiveRepresentation() = default;
};

/**
 * @brief Opaque primitive representation with no additional format metadata.
 */
struct OpaqueRepresentation final : PrimitiveRepresentation {};

/**
 * @brief Convenience descriptor for conventional integer representations.
 */
struct IntegerRepresentation final : PrimitiveRepresentation {
    /** @brief Integer value encoding. */
    IntegerEncoding encoding{IntegerEncoding::TwosComplement};
    /** @brief Byte order of the stored representation. */
    Endianness endianness{Endianness::Native};

    /** @brief Constructs the default two's-complement native-endian representation. */
    IntegerRepresentation() = default;

    /**
     * @brief Constructs an integer representation.
     * @param valueEncoding Integer encoding.
     * @param valueEndianness Byte order.
     */
    IntegerRepresentation(IntegerEncoding valueEncoding, Endianness valueEndianness = Endianness::Native)
        : encoding{valueEncoding}, endianness{valueEndianness} {}
};

/**
 * @brief Convenience descriptor for conventional binary floating-point formats.
 */
struct FloatingPointRepresentation final : PrimitiveRepresentation {
    /** @brief Number of exponent bits. */
    std::size_t exponentBits{0};
    /** @brief Number of significand bits. */
    std::size_t significandBits{0};
    /** @brief Byte order of the stored representation. */
    Endianness endianness{Endianness::Native};

    /** @brief Constructs an empty/default floating-point descriptor. */
    FloatingPointRepresentation() = default;

    /**
     * @brief Constructs a floating-point representation.
     * @param exponent Number of exponent bits.
     * @param significand Number of significand bits.
     * @param valueEndianness Byte order.
     */
    FloatingPointRepresentation(
        std::size_t exponent,
        std::size_t significand,
        Endianness valueEndianness = Endianness::Native
    ) : exponentBits{exponent}, significandBits{significand}, endianness{valueEndianness} {}
};

/**
 * @brief Convenience descriptor for conventional fixed-point formats.
 */
struct FixedPointRepresentation final : PrimitiveRepresentation {
    /** @brief Number of fractional bits. */
    std::size_t fractionalBits{0};
    /** @brief Byte order of the stored representation. */
    Endianness endianness{Endianness::Native};

    /** @brief Constructs an empty/default fixed-point descriptor. */
    FixedPointRepresentation() = default;

    /**
     * @brief Constructs a fixed-point representation.
     * @param fractional Number of fractional bits.
     * @param valueEndianness Byte order.
     */
    FixedPointRepresentation(std::size_t fractional, Endianness valueEndianness = Endianness::Native)
        : fractionalBits{fractional}, endianness{valueEndianness} {}
};

/**
 * @brief Overflow behavior associated with arithmetic semantics.
 */
enum class OverflowBehavior {
    Unspecified,
    Wrap,
    Trap,
    Saturate
};

/**
 * @brief Standard arithmetic metadata that can be attached through ExtensionSet.
 */
struct ArithmeticProperties {
    /** @brief Overflow policy for arithmetic operations. */
    OverflowBehavior overflow{OverflowBehavior::Unspecified};
};

/**
 * @brief Standard numeric metadata that can be attached through ExtensionSet.
 */
struct NumericProperties {
    /** @brief Whether values represent exact quantities. */
    bool exact{true};
    /** @brief Whether values support an ordering relation. */
    bool ordered{true};
};

/**
 * @brief Classification of a registered conversion.
 */
enum class ConversionKind {
    /** May participate automatically in semantic resolution. */
    Implicit,
    /** Requires an explicit language-level conversion. */
    Explicit,
    /** Reinterprets representation according to language-defined semantics. */
    Bitcast
};

/**
 * @brief Registered directed conversion to another type.
 */
struct TypeConversion {
    /** @brief Target type of the directed conversion. */
    TypeId target{};
    /** @brief Conversion category. */
    ConversionKind kind{ConversionKind::Explicit};
    /** @brief Relative conversion cost used by overload resolution. */
    std::size_t rank{1};
    /** @brief Language-defined metadata attached to the conversion. */
    ExtensionSet extensions{};

    /**
     * @brief Reports whether this conversion may be inserted implicitly.
     * @return true when kind is ConversionKind::Implicit.
     */
    bool isImplicit() const noexcept { return kind == ConversionKind::Implicit; }
};

/**
 * @brief Built-in descriptor for primitive scalar types.
 */
class PrimitiveType final : public TypeDefinition {
public:
    /**
     * @brief Constructs a primitive descriptor.
     * @param typeId Identifier of the primitive type.
     */
    explicit PrimitiveType(TypeId typeId = {}) : TypeDefinition{std::move(typeId)} {}

    /** @brief Exact number of bits occupied by a value of this type. */
    std::size_t bits{0};
    /** @brief Exact storage alignment in bits. */
    std::size_t alignmentBits{1};
    /** @brief Signedness classification of the primitive. */
    PrimitiveSignedness signedness{PrimitiveSignedness::NotApplicable};
    /** @brief Optional representation descriptor for the type bits. */
    std::shared_ptr<const PrimitiveRepresentation> representation{std::make_shared<OpaqueRepresentation>()};

    /**
     * @brief Returns the byte count required to contain the type bits.
     * @return Bit width rounded up to full bytes.
     */
    std::size_t byteWidth() const noexcept { return (bits + 7U) / 8U; }
};

} // namespace novac::assets::types