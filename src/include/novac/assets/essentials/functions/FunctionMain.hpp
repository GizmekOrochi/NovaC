#pragma once

#include "novac/assets/essentials/EssentialFeature.hpp"

namespace novac::assets::essentials::functions {

/**
 * @brief Installs program entry-point integration.
 *
 * Registers the behavior required to discover and execute the configured language entry point through the function runtime.
 */
class FunctionMainFeature final : public EssentialFeature {
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
 * @brief Creates a pack containing function entry-point support.
 *
 * @return Ownable feature pack.
 */
EssentialPack functionMain();

}