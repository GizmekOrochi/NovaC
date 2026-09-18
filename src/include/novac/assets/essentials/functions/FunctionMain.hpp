#pragma once

#include "novac/assets/essentials/EssentialFeature.hpp"

namespace novac::assets::essentials::functions {

/**
 * @brief Configures the program entry-point function.
 *
 * FunctionMainFeature connects the configured function syntax options to the
 * function registry by selecting the language-level function used as the
 * program entry point.
 *
 * The feature does not execute the entry point itself. It only records the
 * configured function name in FunctionRegistry.
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
     * @brief Installs entry-point configuration into an Essentials controller.
     *
     * The configured main function name is read from FunctionSyntaxOptions and
     * stored in the controller's FunctionRegistry.
     *
     * @param controller Controller receiving the entry-point configuration.
     */
    void install(EssentialsController &controller) const override;
};

/**
 * @brief Creates a pack containing function entry-point support.
 *
 * The returned pack contains one FunctionMainFeature instance.
 *
 * @return Ownable feature pack.
 */
EssentialPack functionMain();

}
