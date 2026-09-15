#pragma once

#include "novac/assets/essentials/EssentialFeature.hpp"

namespace novac::assets::essentials::variables {

/**
 * @brief Installs standalone expression statements.
 *
 * Registers statement fallback parsing and runtime evaluation for expressions terminated by the configured statement separator.
 */
class ExpressionStatementsFeature final : public EssentialFeature {
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
 * @brief Creates a pack containing expression-statement support.
 *
 * @return Ownable feature pack.
 */
EssentialPack expressionStatements();
/**
 * @brief Creates the standard variable-related pack exposed by this module.
 *
 * @return Ownable feature pack.
 */
EssentialPack standard();

} // namespace novac::assets::essentials::variables
