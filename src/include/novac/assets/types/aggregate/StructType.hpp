#pragma once

#include "novac/assets/types/TypeController.hpp"
#include "novac/assets/types/model/Capabilities.hpp"

#include <string_view>
#include <vector>

namespace novac::assets::types::aggregate {

/** Reference complex type: an ordered collection of named fields. */
class StructType final : public TypeDefinition {
public:
    /**
     * @brief Constructs a `StructType` instance.
     *
     * @param id Value supplied for `id`.
     */
    explicit StructType(TypeId id);

    /**
     * @brief Returns the value exposed by `fields`.
     *
     * @return Value produced by the operation.
     */
    std::span<const TypeComponent> fields() const noexcept { return fields_; }

private:
    friend class StructBuilder;
    std::vector<TypeComponent> fields_{};
};

/** Exposes StructType fields through the generic composition interface. */
class StructComposition final : public CompositionCapability {
public:
    /**
     * @brief Returns the value exposed by `components`.
     *
     * @param type Value supplied for `type`.
     * @return Value produced by the operation.
     */
    std::span<const TypeComponent> components(const TypeDefinition &type) const override;
};

/** Conventional ordered aggregate layout with alignment and tail padding. */
class NaturalStructLayout final : public LayoutCapability {
public:
    /**
     * @brief Computes the result of `compute`.
     *
     * @param context Value supplied for `context`.
     * @param type Value supplied for `type`.
     * @return Value produced by the operation.
     */
    TypeLayout compute(const LayoutContext &context, const TypeDefinition &type) const override;
};

/** Named field lookup implemented through CompositionCapability. */
class NamedMemberAccess final : public MemberCapability {
public:
    /**
     * @brief Finds the value requested by `findMember`.
     *
     * @param type Value supplied for `type`.
     * @param name Value supplied for `name`.
     * @return Value produced by the operation.
     */
    std::optional<TypeMember> findMember(const TypeDefinition &type, std::string_view name) const override;
};

/** Convenience builder for the reference StructType implementation. */
class StructBuilder final {
public:
    /**
     * @brief Constructs a `StructBuilder` instance.
     *
     * @param types Value supplied for `types`.
     * @param id Value supplied for `id`.
     */
    StructBuilder(TypeController &types, TypeId id);
    /**
     * @brief Constructs a `StructBuilder` instance.
     *
     * @param types Value supplied for `types`.
     * @param name Value supplied for `name`.
     */
    StructBuilder(TypeController &types, std::string name) : StructBuilder(types, TypeId{std::move(name)}) {}

    /**
     * @brief Performs the `field` operation.
     *
     * @param name Value supplied for `name`.
     * @param type Value supplied for `type`.
     * @return Value produced by the operation.
     */
    StructBuilder &field(std::string name, TypeId type);

    /**
     * @brief Performs the `extension` operation.
     *
     * @param args Value supplied for `args`.
     * @return Value produced by the operation.
     */
    template <typename Extension, typename... Args>
    StructBuilder &extension(Args &&...args) {
        type_.extensions.emplace<Extension>(std::forward<Args>(args)...);
        return *this;
    }

    /**
     * @brief Performs the `fieldExtension` operation.
     *
     * @param fieldName Value supplied for `fieldName`.
     * @param args Value supplied for `args`.
     * @return Value produced by the operation.
     */
    template <typename Extension, typename... Args>
    StructBuilder &fieldExtension(std::string_view fieldName, Args &&...args) {
        for (auto &field : type_.fields_) {
            if (field.name == fieldName) {
                field.extensions.emplace<Extension>(std::forward<Args>(args)...);
                return *this;
            }
        }
        throw std::runtime_error("StructBuilder::fieldExtension: unknown field '" + std::string{fieldName} + "'");
    }

    /**
     * @brief Performs the `capability` operation.
     *
     * @param args Value supplied for `args`.
     * @return Value produced by the operation.
     */
    template <typename Capability, typename Implementation = Capability, typename... Args>
    StructBuilder &capability(Args &&...args) {
        type_.capabilities.emplace<Capability, Implementation>(std::forward<Args>(args)...);
        return *this;
    }

    /**
     * @brief Returns the value exposed by `definition`.
     *
     * @return Value produced by the operation.
     */
    const StructType &definition() const noexcept { return type_; }
    /**
     * @brief Performs the `commit` operation.
     *
     * @return Value produced by the operation.
     */
    registry::RegisterStatus commit();

private:
    TypeController *types_{nullptr};
    StructType type_;
};

/**
 * @brief Creates a value through `defineStruct`.
 *
 * @param types Value supplied for `types`.
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
inline StructBuilder defineStruct(TypeController &types, TypeId id) {
    return StructBuilder{types, std::move(id)};
}

/**
 * @brief Creates a value through `defineStruct`.
 *
 * @param types Value supplied for `types`.
 * @param name Value supplied for `name`.
 * @return Value produced by the operation.
 */
inline StructBuilder defineStruct(TypeController &types, std::string name) {
    return StructBuilder{types, std::move(name)};
}

} // namespace novac::assets::types::aggregate
