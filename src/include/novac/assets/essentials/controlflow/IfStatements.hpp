#pragma once

#include "novac/assets/essentials/EssentialFeature.hpp"

namespace novac::assets::essentials::controlflow {

/**
 * @brief Installs if/else conditional statements.
 *
 * IfStatementsFeature registers the syntax, AST schema and runtime behavior
 * required for conditional execution.
 *
 * Each generated node contains a required condition and then branch together
 * with an optional else branch.
 */
class IfStatementsFeature final : public EssentialFeature {
public:
    /**
     * @brief Returns metadata describing this feature.
     *
     * The feature provides the configured if-statement node kind and the
     * statement trait.
     *
     * @return Feature registration and dependency information.
     */
    EssentialInfo info() const override;

    /**
     * @brief Installs if/else support into an Essentials controller.
     *
     * Installation registers the configured if and else keywords, creates the
     * conditional AST schema and installs parser and runtime handlers.
     *
     * Parsing reads the condition as an expression, then parses the required
     * statement branch and an optional else statement. Runtime evaluation uses
     * Value::truthy() to select which branch is executed.
     *
     * @param controller Controller receiving parser, schema, and runtime registrations.
     */
    void install(EssentialsController &controller) const override;
};

/**
 * @brief Creates a pack containing if/else statement support.
 *
 * The returned pack contains one IfStatementsFeature instance.
 *
 * @return Ownable feature pack.
 */
EssentialPack ifStatements();

/**
 * @brief Creates the standard control-flow pack exposed by this namespace.
 *
 * @return Ownable feature pack.
 */
EssentialPack standard();

} // namespace novac::assets::essentials::controlflow
