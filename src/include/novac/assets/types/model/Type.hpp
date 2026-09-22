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

struct TypeId {
    std::string name{};

    TypeId() = default;
    explicit TypeId(std::string value) : name{std::move(value)} {}

    bool valid() const noexcept { return !name.empty(); }
    explicit operator bool() const noexcept { return valid(); }

    friend bool operator==(const TypeId &, const TypeId &) = default;
};

struct TypeIdHash {
    std::size_t operator()(const TypeId &id) const noexcept {
        return std::hash<std::string>{}(id.name);
    }
};

/** Typed immutable-after-freeze metadata storage. */
class ExtensionSet final {
public:
    template <typename T, typename... Args>
    const T &emplace(Args &&...args) {
        static_assert(!std::is_reference_v<T>, "extension type cannot be a reference");
        ensureMutable();
        auto value = std::make_shared<const T>(std::forward<Args>(args)...);
        const T &reference = *value;
        values_.insert_or_assign(std::type_index(typeid(T)), std::move(value));
        return reference;
    }

    template <typename T>
    const T &set(T value) {
        return emplace<T>(std::move(value));
    }

    template <typename T>
    bool has() const noexcept {
        return values_.find(std::type_index(typeid(T))) != values_.end();
    }

    template <typename T>
    const T *get() const noexcept {
        const auto it = values_.find(std::type_index(typeid(T)));
        return it == values_.end() ? nullptr : static_cast<const T *>(it->second.get());
    }

    template <typename T>
    void erase() {
        ensureMutable();
        values_.erase(std::type_index(typeid(T)));
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
        if (frozen_) {
            throw std::runtime_error("ExtensionSet: committed metadata is immutable");
        }
    }

    std::unordered_map<std::type_index, std::shared_ptr<const void>> values_{};
    bool frozen_{false};
};

enum class TypeKind {
    Primitive,
    UserDefined
};

/** Base descriptor stored by TypeController. */
class TypeDefinition {
public:
    explicit TypeDefinition(TypeId typeId) : id{std::move(typeId)} {}
    virtual ~TypeDefinition() = default;

    TypeId id{};
    ExtensionSet extensions{};

    virtual TypeKind kind() const noexcept { return TypeKind::UserDefined; }

    void freeze() {
        extensions.freeze();
        onFreeze();
    }

protected:
    virtual void onFreeze() {}
};

enum class PrimitiveSignedness {
    NotApplicable,
    Signed,
    Unsigned
};

enum class Endianness {
    Native,
    Little,
    Big
};

enum class IntegerEncoding {
    PureBinary,
    TwosComplement,
    OnesComplement,
    SignMagnitude
};

class PrimitiveRepresentation {
public:
    virtual ~PrimitiveRepresentation() = default;
};

struct OpaqueRepresentation final : PrimitiveRepresentation {};

struct IntegerRepresentation final : PrimitiveRepresentation {
    IntegerEncoding encoding{IntegerEncoding::TwosComplement};
    Endianness endianness{Endianness::Native};

    IntegerRepresentation() = default;
    IntegerRepresentation(IntegerEncoding valueEncoding, Endianness valueEndianness = Endianness::Native)
        : encoding{valueEncoding}, endianness{valueEndianness} {}
};

struct FloatingPointRepresentation final : PrimitiveRepresentation {
    std::size_t exponentBits{0};
    std::size_t significandBits{0};
    Endianness endianness{Endianness::Native};

    FloatingPointRepresentation() = default;
    FloatingPointRepresentation(
        std::size_t exponent,
        std::size_t significand,
        Endianness valueEndianness = Endianness::Native
    ) : exponentBits{exponent}, significandBits{significand}, endianness{valueEndianness} {}
};

struct FixedPointRepresentation final : PrimitiveRepresentation {
    std::size_t fractionalBits{0};
    Endianness endianness{Endianness::Native};

    FixedPointRepresentation() = default;
    FixedPointRepresentation(std::size_t fractional, Endianness valueEndianness = Endianness::Native)
        : fractionalBits{fractional}, endianness{valueEndianness} {}
};

enum class OverflowBehavior {
    Unspecified,
    Wrap,
    Trap,
    Saturate
};

struct ArithmeticProperties {
    OverflowBehavior overflow{OverflowBehavior::Unspecified};
};

struct NumericProperties {
    bool exact{true};
    bool ordered{true};
};

enum class ConversionKind {
    Implicit,
    Explicit,
    Bitcast
};

struct TypeConversion {
    TypeId target{};
    ConversionKind kind{ConversionKind::Explicit};
    std::size_t rank{1};
    ExtensionSet extensions{};

    bool isImplicit() const noexcept { return kind == ConversionKind::Implicit; }
};

class PrimitiveType final : public TypeDefinition {
public:
    explicit PrimitiveType(TypeId typeId = {}) : TypeDefinition{std::move(typeId)} {}

    TypeKind kind() const noexcept override { return TypeKind::Primitive; }

    std::size_t bitWidth{0};
    std::size_t storageBits{0};
    std::size_t alignment{1};
    PrimitiveSignedness signedness{PrimitiveSignedness::NotApplicable};
    std::shared_ptr<const PrimitiveRepresentation> representation{
        std::make_shared<OpaqueRepresentation>()
    };

    std::size_t valueByteWidth() const noexcept { return (bitWidth + 7U) / 8U; }
    std::size_t storageByteWidth() const noexcept { return (storageBits + 7U) / 8U; }
    std::size_t byteWidth() const noexcept { return storageByteWidth(); }
};

} // namespace novac::assets::types
