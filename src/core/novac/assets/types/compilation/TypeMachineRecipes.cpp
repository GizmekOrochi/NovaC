#include "novac/assets/types/compilation/TypeMachineRecipes.hpp"

namespace novac::assets::types::compilation {
void addPrimitiveLayout(novac::compilation::machine::MachineLayoutRegistry &out,
                        IRTypeAdapter &typeAdapter, const TypeController &types, TypeId type) {
    const auto &primitive = types.requirePrimitive(type);
    using novac::compilation::machine::ScalarKind;
    ScalarKind kind;
    switch (primitive.signedness) {
        case PrimitiveSignedness::Signed: kind=ScalarKind::SignedInteger; break;
        case PrimitiveSignedness::Unsigned: kind=ScalarKind::UnsignedInteger; break;
        default: throw std::runtime_error("TypeMachineRecipes: primitive has no machine integer signedness");
    }
    if (primitive.bits > 255 || primitive.alignmentBits > 255 || primitive.alignmentBits % 8 != 0)
        throw std::runtime_error("TypeMachineRecipes: unsupported primitive width/alignment");
    out.add(typeAdapter.toIR(std::move(type)),
            {static_cast<std::uint8_t>(primitive.bits),kind,
             static_cast<std::uint8_t>(primitive.alignmentBits / 8)});
}
void addTypedRecipe(novac::compilation::machine::MachineRecipeRegistry &out,
                    IRTypeAdapter &typeAdapter, TypeId type,
                    novac::compilation::machine::MachineRecipe recipe) {
    recipe.resultType = typeAdapter.toIR(std::move(type));
    out.add(std::move(recipe));
}
} // namespace novac::assets::types::compilation
