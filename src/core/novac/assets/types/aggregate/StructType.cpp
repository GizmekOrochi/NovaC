#include "novac/assets/types/aggregate/StructType.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace novac::assets::types::aggregate {
namespace {

std::size_t alignUp(std::size_t value, std::size_t alignment) {
    if (alignment == 0)
        throw std::runtime_error("NaturalStructLayout: zero alignment");
    const std::size_t remainder{value % alignment};
    if (remainder == 0)
        return value;
    const std::size_t padding{alignment - remainder};
    if (value > std::numeric_limits<std::size_t>::max() - padding)
        throw std::runtime_error("NaturalStructLayout: layout size overflow");
    return value + padding;
}

const CompositionCapability &requireComposition(const TypeDefinition &type, const char *owner) {
    const auto *composition{type.capabilities.get<CompositionCapability>()};
    if (!composition)
        throw std::runtime_error(std::string{owner} + ": type '" + type.id.name + "' has no composition capability");
    return *composition;
}

} // namespace

StructType::StructType(TypeId id) : TypeDefinition{std::move(id)} {
    capabilities.emplace<CompositionCapability, StructComposition>();
    capabilities.emplace<LayoutCapability, NaturalStructLayout>();
    capabilities.emplace<MemberCapability, NamedMemberAccess>();
}

std::span<const TypeComponent> StructComposition::components(const TypeDefinition &type) const {
    const auto *structure{dynamic_cast<const StructType *>(&type)};
    if (!structure)
        throw std::runtime_error("StructComposition: capability attached to a non-StructType descriptor");
    return structure->fields();
}

TypeLayout NaturalStructLayout::compute(const LayoutContext &context, const TypeDefinition &type) const {
    const auto &composition{requireComposition(type, "NaturalStructLayout")};
    const auto components{composition.components(type)};

    TypeLayout result{};
    result.alignmentBits = 1;
    result.components.reserve(components.size());

    std::size_t cursorBits{0};
    for (std::size_t index{0}; index < components.size(); ++index) {
        const TypeLayout child{context.layoutOf(components[index].type)};
        cursorBits = alignUp(cursorBits, child.alignmentBits);
        result.components.push_back(ComponentLayout{index, cursorBits, child.bitSize, child.alignmentBits});
        if (cursorBits > std::numeric_limits<std::size_t>::max() - child.bitSize)
            throw std::runtime_error("NaturalStructLayout: layout size overflow");
        cursorBits += child.bitSize;
        result.alignmentBits = std::max(result.alignmentBits, child.alignmentBits);
    }

    result.bitSize = alignUp(cursorBits, result.alignmentBits);
    return result;
}

std::optional<TypeMember> NamedMemberAccess::findMember(const TypeDefinition &type, std::string_view name) const {
    const auto &composition{requireComposition(type, "NamedMemberAccess")};
    const auto components{composition.components(type)};
    for (std::size_t index{0}; index < components.size(); ++index) {
        if (components[index].name == name)
            return TypeMember{&components[index], index};
    }
    return std::nullopt;
}

StructBuilder::StructBuilder(TypeController &types, TypeId id) : types_{&types}, type_{std::move(id)} {}

StructBuilder &StructBuilder::field(std::string name, TypeId type) {
    if (name.empty())
        throw std::runtime_error("StructBuilder::field: field name cannot be empty");
    if (!type.valid())
        throw std::runtime_error("StructBuilder::field: field type cannot be empty");
    if (std::any_of(type_.fields_.begin(), type_.fields_.end(), [&](const TypeComponent &field) { return field.name == name; }))
        throw std::runtime_error("StructBuilder::field: duplicate field '" + name + "'");
    type_.fields_.push_back(TypeComponent{std::move(name), std::move(type), {}});
    return *this;
}

registry::RegisterStatus StructBuilder::commit() {
    if (!types_)
        throw std::runtime_error("StructBuilder::commit: builder has no TypeController");

    for (auto &field : type_.fields_) {
        field.type = types_->canonical(std::move(field.type));
        if (!(field.type == type_.id) && !types_->isDeclared(field.type))
            throw std::runtime_error("StructBuilder::commit: field '" + field.name + "' references unknown type '" + field.type.name + "'");
        field.extensions.freeze();
    }

    return types_->registerType(std::make_unique<StructType>(std::move(type_)));
}

} // namespace novac::assets::types::aggregate
