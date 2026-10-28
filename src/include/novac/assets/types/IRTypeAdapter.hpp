#pragma once

#include "novac/assets/types/TypeController.hpp"
#include "novac/engine/transformation/TypeIdentity.hpp"

#include <string>
#include <utility>

namespace novac::assets::types {

/** Optional bridge; no dependency from Engine to the Types asset.
 * A canonical TypeId is mapped to one stable, scoped IR identity. */
class IRTypeAdapter final {
public:
    IRTypeAdapter(const TypeController &types, std::string scope)
        : types_{types}, space_{std::move(scope)} {}

    ir::TypeRef toIR(TypeId source) {
        if (!source.valid() || !types_.hasType(source))
            throw std::runtime_error("IRTypeAdapter: unknown TypeId");
        return space_.intern(types_.canonical(std::move(source)).name);
    }
    const ir::TypeSpace &space() const noexcept { return space_; }

private:
    const TypeController &types_;
    ir::TypeSpace space_;
};

} // namespace novac::assets::types
