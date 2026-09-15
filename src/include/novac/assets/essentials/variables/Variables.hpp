#pragma once

#include "novac/assets/essentials/EssentialFeature.hpp"

namespace novac::assets::essentials::variables {

/**
 * @brief Installs variable declarations, assignments, and lookups.
 *
 * Registers variable syntax, AST schemas, environment mutation, identifier lookup, and assignment behavior.
 */
class VariablesFeature final : public EssentialFeature {
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
 * @brief Creates a pack containing variable support.
 *
 * @return Ownable feature pack.
 */
EssentialPack variables();
/**
 * @brief Creates the standard variable pack, including expression statements.
 *
 * @return Ownable feature pack.
 */
EssentialPack standard();

} // namespace novac::assets::essentials::variables
