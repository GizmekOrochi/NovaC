#include "novac/assets/types/LayoutController.hpp"

#include <stdexcept>
#include <unordered_set>

namespace novac::assets::types {
namespace {

class ResolutionContext final : public LayoutContext {
public:
    explicit ResolutionContext(const TypeController &types) : types_{types} {}

    const TypeController &types() const noexcept override { return types_; }

    TypeLayout layoutOf(const TypeId &id) const override {
        const TypeId canonical{types_.canonical(id)};
        if (!types_.hasType(canonical))
            throw std::runtime_error("LayoutController::compute: unknown type '" + id.name + "'");

        if (!active_.insert(canonical).second)
            throw std::runtime_error("LayoutController::compute: recursive by-value layout involving type '" + canonical.name + "'");

        struct Guard {
            std::unordered_set<TypeId, TypeIdHash> &active;
            TypeId id;
            ~Guard() { active.erase(id); }
        } guard{active_, canonical};

        const TypeDefinition &type{types_.requireType(canonical)};
        if (const auto *primitive{dynamic_cast<const PrimitiveType *>(&type)}) {
            return TypeLayout{primitive->storageByteWidth(), primitive->alignment, {}, {}};
        }

        const auto *layout{type.capabilities.get<LayoutCapability>()};
        if (!layout)
            throw std::runtime_error("LayoutController::compute: type '" + canonical.name + "' has no layout capability");

        TypeLayout result{layout->compute(*this, type)};
        if (result.alignment == 0)
            throw std::runtime_error("LayoutController::compute: type '" + canonical.name + "' produced zero alignment");
        result.extensions.freeze();
        return result;
    }

private:
    const TypeController &types_;
    mutable std::unordered_set<TypeId, TypeIdHash> active_{};
};

} // namespace

TypeLayout LayoutController::compute(const TypeId &type) const {
    if (!types_)
        throw std::runtime_error("LayoutController::compute: controller is not initialized");
    ResolutionContext context{*types_};
    return context.layoutOf(type);
}

bool LayoutController::hasLayout(const TypeId &type) const {
    if (!types_ || !types_->hasType(type))
        return false;
    const TypeDefinition &definition{types_->requireType(type)};
    return dynamic_cast<const PrimitiveType *>(&definition) != nullptr || definition.capabilities.has<LayoutCapability>();
}

} // namespace novac::assets::types
