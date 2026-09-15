#pragma once

#include "novac/assets/essentials/EssentialFeature.hpp"
#include "novac/assets/essentials/functions/FunctionRegistry.hpp"

namespace novac::assets::essentials::functions {

/**
 * @brief Installs function declarations and calls.
 *
 * Registers function declaration and call syntax, AST schemas, user-defined invocation, argument binding, recursion support, and native-function dispatch.
 */
class FunctionsFeature final : public EssentialFeature {
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
 * @brief Creates a pack containing function declaration and call support.
 *
 * @return Ownable feature pack.
 */
EssentialPack functions();
/**
 * @brief Creates the standard function feature pack.
 *
 * @return Ownable feature pack.
 */
EssentialPack standard();

} // namespace novac::assets::essentials::functions
