#pragma once

#include "novac/assets/essentials/EssentialFeature.hpp"

namespace novac::assets::essentials::scopes {

/**
 * @brief Installs lexically scoped statement blocks.
 *
 * Registers block parsing, AST schemas, and runtime scope push/pop behavior for brace-delimited statement blocks.
 */
class ScopedBlocksFeature final : public EssentialFeature {
public:
    /**
     * @brief Returns metadata describing this feature.
     *
     * @return Feature registration and dependency information.
     */
    EssentialInfo info() const override;
    /**
     * @brief Installs the feature into an Essentials controller.
     *
     * @param controller Controller receiving parser, schema, and runtime registrations.
     */
    void install(EssentialsController &controller) const override;
};

/**
 * @brief Creates a pack containing scoped-block support.
 *
 * @return Ownable feature pack.
 */
EssentialPack scopedBlocks();
/**
 * @brief Creates the standard scope feature pack.
 *
 * @return Ownable feature pack.
 */
EssentialPack standard();

} // namespace novac::assets::essentials::scopes
