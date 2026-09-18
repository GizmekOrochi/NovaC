#pragma once

#include "novac/assets/essentials/EssentialFeature.hpp"

namespace novac::assets::essentials::variables {

/**
 * @brief Installs variable declarations, assignments and lookups.
 *
 * VariablesFeature registers the syntax, AST schemas and runtime behavior
 * required for local variables.
 *
 * Variable declarations create bindings in the current environment.
 * Assignments update the nearest existing binding when possible and otherwise
 * define a new binding in the current scope.
 *
 * Identifier expressions are also used as the entry point for parsing function
 * calls when an opening parenthesis follows the identifier.
 */
class VariablesFeature final : public EssentialFeature {
public:
    /**
     * @brief Returns metadata describing this feature.
     *
     * The feature provides variable declaration, variable expression and
     * assignment statement node kinds together with expression and statement
     * capabilities.
     *
     * @return Feature registration and dependency information.
     */
    EssentialInfo info() const override;

    /**
     * @brief Installs variable support into an Essentials controller.
     *
     * Installation registers the variable declaration keyword, assignment
     * token and statement terminator, then creates AST schemas for declarations,
     * assignments and variable lookup expressions.
     *
     * Declaration parsing requires a name, assignment token, expression and
     * terminator. Assignment parsing is installed as a statement fallback and
     * only activates when the current token is an identifier immediately
     * followed by the configured assignment token.
     *
     * Identifier expressions normally produce variable lookup nodes. When the
     * identifier is followed by an opening parenthesis, the parser instead
     * builds a function-call node and parses its argument list.
     *
     * At runtime, declarations reject duplicate names in the current scope.
     * Assignments update an existing binding through the environment chain and
     * create a new current-scope binding when no previous binding exists.
     * Variable lookups search the active environment chain and throw when the
     * requested name cannot be resolved.
     *
     * @param controller Controller receiving parser, schema, and runtime registrations.
     */
    void install(EssentialsController &controller) const override;
};

/**
 * @brief Creates a pack containing variable support.
 *
 * The returned pack contains one VariablesFeature instance.
 *
 * @return Ownable feature pack.
 */
EssentialPack variables();

/**
 * @brief Creates the standard variable feature pack.
 *
 * The standard pack combines variable declarations, assignments and lookups
 * with expression-statement support.
 *
 * @return Ownable feature pack.
 */
EssentialPack standard();

} // namespace novac::assets::essentials::variables
