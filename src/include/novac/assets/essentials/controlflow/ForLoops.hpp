#pragma once

#include "novac/assets/essentials/EssentialFeature.hpp"

namespace novac::assets::essentials::controlflow {

/**
 * @brief Installs C-style for loops.
 *
 * Registers parsing, AST schema generation, and runtime execution for C-style for loops, including initializer, condition, step, body, and iteration-limit handling.
 */
class ForLoopsFeature final : public EssentialFeature {
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
 * @brief Creates a pack containing for-loop support.
 *
 * @return Ownable feature pack.
 */
EssentialPack forLoops();
/**
 * @brief Creates the standard control-flow pack exposed by this module.
 *
 * @return Ownable feature pack.
 */
EssentialPack standard();

} // namespace novac::assets::essentials::controlflow
