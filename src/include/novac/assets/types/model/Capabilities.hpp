#pragma once

#include "novac/assets/types/model/Type.hpp"

#include <cstddef>
#include <optional>
#include <span>
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

/** Layout of one logical component. */
struct ComponentLayout {
    std::size_t componentIndex{0};
    std::size_t offset{0};
    std::size_t size{0};
    std::size_t alignment{1};
};

/** Physical layout resolved for a type. */
struct TypeLayout {
    std::size_t size{0};
    std::size_t alignment{1};
    std::vector<ComponentLayout> components{};
    ExtensionSet extensions{};
};

/** Recursive layout context passed to custom layout behaviors. */
class LayoutContext {
public:
    virtual ~LayoutContext() = default;
    virtual const TypeController &types() const noexcept = 0;
    virtual TypeLayout layoutOf(const TypeId &type) const = 0;
};

/** Behavior for a type that can produce a physical layout. */
class LayoutCapability : public TypeCapability {
public:
    virtual ~LayoutCapability() = default;
    virtual TypeLayout compute(const LayoutContext &context, const TypeDefinition &type) const = 0;
};

/** Semantic member exposed by a type. Members need not correspond to storage fields. */
struct TypeMember {
    std::string name{};
    TypeId type{};
    std::size_t componentIndex{0};
};

/** Behavior for named member lookup. */
class MemberCapability : public TypeCapability {
public:
    virtual ~MemberCapability() = default;
    virtual std::optional<TypeMember> findMember(const TypeDefinition &type, std::string_view name) const = 0;
};

} // namespace novac::assets::types
