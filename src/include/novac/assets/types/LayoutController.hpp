#pragma once

#include "novac/assets/types/TypeController.hpp"
#include "novac/assets/types/model/Capabilities.hpp"

#include <unordered_map>
#include <unordered_set>

namespace novac::assets::types {

/**
 * @brief Reusable layout-resolution session with recursion tracking and caching.
 *
 * A session keeps one cache for an entire higher-level operation. Custom
 * capabilities that recursively ask for other layouts therefore reuse the same
 * resolved descriptors instead of starting independent computations.
 */
class LayoutResolutionSession final : public LayoutContext {
public:
    explicit LayoutResolutionSession(const TypeController &types) : types_{&types} {}

    /** @brief Returns the type registry used by this session. */
    const TypeController &types() const noexcept override { return *types_; }

    /**
     * @brief Resolves a type layout using this session's recursion guard/cache.
     * @param type Type or alias to resolve.
     * @return Resolved layout.
     */
    TypeLayout layoutOf(const TypeId &type) const override;

private:
    const TypeController *types_{nullptr};
    mutable std::unordered_set<TypeId, TypeIdHash> active_{};
    mutable std::unordered_map<TypeId, TypeLayout, TypeIdHash> cache_{};
};

/** Resolves type layouts without embedding ABI policy in TypeController. */
class LayoutController final {
public:
    explicit LayoutController(const TypeController &types) : types_{&types} {}

    /** @brief Creates a reusable resolution session for one higher-level operation. */
    LayoutResolutionSession session() const;

    TypeLayout compute(const TypeId &type) const;
    bool hasLayout(const TypeId &type) const;

private:
    const TypeController *types_{nullptr};
};

} // namespace novac::assets::types
