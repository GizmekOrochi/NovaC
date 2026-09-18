#pragma once

#include "novac/assets/essentials/EssentialFeature.hpp"

namespace novac::assets::essentials::controlflow {

/**
 * @brief Installs C-style for loops.
 *
 * ForLoopsFeature registers the syntax, AST schema and runtime behavior needed
 * for loops with an optional initializer, optional condition, optional step
 * expression and a required body.
 *
 * The initializer and step are parsed as assignment statements without their
 * own trailing semicolon. Runtime execution creates a dedicated scope for the
 * complete loop.
 */
class ForLoopsFeature final : public EssentialFeature {
public:
    /**
     * @brief Returns metadata describing this feature.
     *
     * The feature provides the configured for-loop statement node kind and the
     * statement trait.
     *
     * @return Feature registration and dependency information.
     */
    EssentialInfo info() const override;

    /**
     * @brief Installs for-loop support into an Essentials controller.
     *
     * Installation registers the configured for keyword, creates the for-loop
     * AST schema and installs parser and runtime handlers.
     *
     * Parsing supports omitted initializer, condition and step sections. At
     * runtime, the initializer runs once, the condition is checked before every
     * iteration, and the step runs after the body. An omitted condition behaves
     * as an always-true condition.
     *
     * The loop executes inside a nested runtime scope. The scope is removed
     * both after normal completion and when an exception is thrown.
     *
     * Execution stops when a pending return value is detected. When
     * maxLoopIterations is greater than zero, exceeding that limit throws a
     * runtime error.
     *
     * @param controller Controller receiving parser, schema, and runtime registrations.
     */
    void install(EssentialsController &controller) const override;
};

/**
 * @brief Creates a pack containing for-loop support.
 *
 * The returned pack contains one ForLoopsFeature instance.
 *
 * @return Ownable feature pack.
 */
EssentialPack forLoops();

/**
 * @brief Creates the standard control-flow feature pack.
 *
 * The standard pack combines if/else statements, while loops and for loops.
 *
 * @return Ownable feature pack.
 */
EssentialPack standard();

} // namespace novac::assets::essentials::controlflow
