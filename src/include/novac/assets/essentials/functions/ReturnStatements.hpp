#pragma once

#include "novac/assets/essentials/EssentialFeature.hpp"

namespace novac::assets::essentials::functions {

/**
 * @brief Installs function return statements.
 *
 * Registers return-statement parsing, AST schema generation, and runtime return propagation for functions.
 */
class ReturnStatementsFeature final : public EssentialFeature {
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
 * @brief Creates a pack containing return-statement support.
 *
 * @return Ownable feature pack.
 */
EssentialPack returnStatements();
/**
 * @brief Creates the standard return feature pack exposed by this module.
 *
 * @return Ownable feature pack.
 */
EssentialPack standard();

} // namespace novac::assets::essentials::functions
