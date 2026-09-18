#pragma once

#include "novac/assets/essentials/EssentialFeature.hpp"

namespace novac::assets::essentials::functions {

/**
 * @brief Installs function return statements.
 *
 * ReturnStatementsFeature registers the return keyword, return-statement AST
 * schema, parser rule and runtime handler used for function return propagation.
 *
 * A return statement may optionally contain an expression. At runtime the
 * expression is evaluated and stored in RuntimeContext as the pending return
 * value. A return without an expression produces the runtime void value.
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
     * @brief Installs return-statement support into an Essentials controller.
     *
     * Parsing consumes the configured return keyword, optionally parses an
     * expression before the statement terminator, and creates the configured
     * return AST node.
     *
     * The runtime handler evaluates the optional expression and forwards the
     * resulting value to RuntimeContext::returnValue().
     *
     * @param controller Controller receiving parser, schema, and runtime registrations.
     */
    void install(EssentialsController &controller) const override;
};

/**
 * @brief Creates a pack containing return-statement support.
 *
 * The returned pack contains one ReturnStatementsFeature instance.
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
