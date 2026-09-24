#pragma once

#include "novac/assets/types/TypeController.hpp"
#include "novac/assets/types/model/Capabilities.hpp"

#include <string_view>
#include <vector>

namespace novac::assets::types::aggregate {

/** Reference complex type: an ordered collection of named fields. */
class StructType final : public TypeDefinition {
public:
    explicit StructType(TypeId id);

    std::span<const TypeComponent> fields() const noexcept { return fields_; }

private:
    friend class StructBuilder;
    std::vector<TypeComponent> fields_{};
};

/** Exposes StructType fields through the generic composition interface. */
class StructComposition final : public CompositionCapability {
public:
    std::span<const TypeComponent> components(const TypeDefinition &type) const override;
};

/** Conventional ordered aggregate layout with alignment and tail padding. */
class NaturalStructLayout final : public LayoutCapability {
public:
    TypeLayout compute(const LayoutContext &context, const TypeDefinition &type) const override;
};

/** Named field lookup implemented through CompositionCapability. */
class NamedMemberAccess final : public MemberCapability {
public:
    std::optional<TypeMember> findMember(const TypeDefinition &type, std::string_view name) const override;
};

/** Convenience builder for the reference StructType implementation. */
class StructBuilder final {
public:
    StructBuilder(TypeController &types, TypeId id);
    StructBuilder(TypeController &types, std::string name) : StructBuilder(types, TypeId{std::move(name)}) {}

    StructBuilder &field(std::string name, TypeId type);

    template <typename Extension, typename... Args>
    StructBuilder &extension(Args &&...args) {
        type_.extensions.emplace<Extension>(std::forward<Args>(args)...);
        return *this;
    }

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

    template <typename Capability, typename Implementation = Capability, typename... Args>
    StructBuilder &capability(Args &&...args) {
        type_.capabilities.emplace<Capability, Implementation>(std::forward<Args>(args)...);
        return *this;
    }

    const StructType &definition() const noexcept { return type_; }
    registry::RegisterStatus commit();

private:
    TypeController *types_{nullptr};
    StructType type_;
};

inline StructBuilder defineStruct(TypeController &types, TypeId id) {
    return StructBuilder{types, std::move(id)};
}

inline StructBuilder defineStruct(TypeController &types, std::string name) {
    return StructBuilder{types, std::move(name)};
}

} // namespace novac::assets::types::aggregate
