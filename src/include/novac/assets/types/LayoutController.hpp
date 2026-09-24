#pragma once

#include "novac/assets/types/TypeController.hpp"
#include "novac/assets/types/model/Capabilities.hpp"

namespace novac::assets::types {

/** Resolves physical layouts without embedding ABI policy in TypeController. */
class LayoutController final {
public:
    explicit LayoutController(const TypeController &types) : types_{&types} {}

    TypeLayout compute(const TypeId &type) const;
    bool hasLayout(const TypeId &type) const;

private:
    const TypeController *types_{nullptr};
};

} // namespace novac::assets::types
