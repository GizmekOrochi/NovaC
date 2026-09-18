#pragma once

#include "novac/assets/essentials/EssentialFeature.hpp"

namespace novac::assets::essentials::variables {

/**
 * @brief Installs expression statements.
 *
 * ExpressionStatementsFeature allows any expression followed by the configured
 * statement terminator to be used as a statement.
 *
 * The parser is registered as a fallback for the statement domain. It first
 * parses an expression and accepts the construct only when the expected
 * terminator follows it.
 */
class ExpressionStatementsFeature final : public EssentialFeature {
public:
    /**
     * @brief Returns metadata describing this feature.
     *
     * The feature provides the configured expression-statement node kind and
     * the statement trait.
     *
     * @return Feature registration and dependency information.
     */
    EssentialInfo info() const override;

    /**
     * @brief Installs expression-statement support into an Essentials controller.
     *
     * Installation registers the statement terminator, creates the expression
     * statement AST schema and installs parser and runtime handlers.
     *
     * The statement parser is registered as a fallback. It parses an
     * expression first and returns nullptr when no terminator follows, allowing
     * other fallback rules to continue.
     *
     * Runtime execution evaluates the stored expression and discards the
     * produced value.
     *
     * @param controller Controller receiving parser, schema, and runtime registrations.
     */
    void install(EssentialsController &controller) const override;
};

/**
 * @brief Creates a pack containing expression-statement support.
 *
 * The returned pack contains one ExpressionStatementsFeature instance.
 *
 * @return Ownable feature pack.
 */
EssentialPack expressionStatements();

/**
 * @brief Creates the standard variable-related feature pack exposed here.
 *
 * @return Ownable feature pack.
 */
EssentialPack standard();

} // namespace novac::assets::essentials::variables
