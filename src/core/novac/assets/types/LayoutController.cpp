#include "novac/assets/types/LayoutController.hpp"

#include <stdexcept>

namespace novac::assets::types {

TypeLayout LayoutResolutionSession::layoutOf(const TypeId &id) const {
    if (!types_)
        throw std::runtime_error("LayoutResolutionSession::layoutOf: session is not initialized");

    const TypeId canonical{types_->canonical(id)};
    if (!types_->hasType(canonical))
        throw std::runtime_error("LayoutController::compute: unknown type '" + id.name + "'");

    if (const auto cached{cache_.find(canonical)}; cached != cache_.end())
        return cached->second;

    if (!active_.insert(canonical).second)
        throw std::runtime_error("LayoutController::compute: recursive by-value layout involving type '" + canonical.name + "'");

    struct Guard {
        std::unordered_set<TypeId, TypeIdHash> &active;
        TypeId id;
        ~Guard() { active.erase(id); }
    } guard{active_, canonical};

    const TypeDefinition &type{types_->requireType(canonical)};
    TypeLayout result{};

    const auto *primitive{dynamic_cast<const PrimitiveType *>(&type)};
    if (const auto *layout{type.capabilities.get<LayoutCapability>()}) {
        result = layout->compute(*this, type);
        if (primitive && result.bitSize != primitive->bits) {
            throw std::runtime_error(
                "LayoutController::compute: primitive '" + canonical.name +
                "' layout size " + std::to_string(result.bitSize) +
                " bits does not match its declared " + std::to_string(primitive->bits) + " bits");
        }
    } else if (primitive) {
        result.bitSize = primitive->bits;
        result.alignmentBits = primitive->alignmentBits;
    } else {
        throw std::runtime_error("LayoutController::compute: type '" + canonical.name + "' has no layout capability");
    }

    if (result.alignmentBits == 0)
        throw std::runtime_error("LayoutController::compute: type '" + canonical.name + "' produced zero alignment");

    result.extensions.freeze();
    cache_.insert_or_assign(canonical, result);
    return result;
}

LayoutResolutionSession LayoutController::session() const {
    if (!types_)
        throw std::runtime_error("LayoutController::session: controller is not initialized");
    return LayoutResolutionSession{*types_};
}

TypeLayout LayoutController::compute(const TypeId &type) const {
    auto resolution{session()};
    return resolution.layoutOf(type);
}

bool LayoutController::hasLayout(const TypeId &type) const {
    if (!types_ || !types_->hasType(type))
        return false;
    const TypeDefinition &definition{types_->requireType(type)};
    return definition.capabilities.has<LayoutCapability>() || dynamic_cast<const PrimitiveType *>(&definition) != nullptr;
}

} // namespace novac::assets::types
