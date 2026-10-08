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
    /**
     * @brief Constructs a `LayoutResolutionSession` instance.
     *
     * @param types Value supplied for `types`.
     */
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
    /**
     * @brief Constructs a `LayoutController` instance.
     *
     * @param types Value supplied for `types`.
     */
    explicit LayoutController(const TypeController &types) : types_{&types} {}

    /** @brief Creates a reusable resolution session for one higher-level operation. */
    LayoutResolutionSession session() const;

    /**
     * @brief Computes the result of `compute`.
     *
     * @param type Value supplied for `type`.
     * @return Value produced by the operation.
     */
    TypeLayout compute(const TypeId &type) const;
    /**
     * @brief Checks the condition represented by `hasLayout`.
     *
     * @param type Value supplied for `type`.
     * @return Value produced by the operation.
     */
    bool hasLayout(const TypeId &type) const;

private:
    const TypeController *types_{nullptr};
};

} // namespace novac::assets::types
