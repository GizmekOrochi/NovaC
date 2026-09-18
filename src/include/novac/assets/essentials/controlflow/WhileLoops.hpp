#pragma once

#include "novac/assets/essentials/EssentialFeature.hpp"

namespace novac::assets::essentials::controlflow {

/**
 * @brief Installs while loops.
 *
 * WhileLoopsFeature registers the syntax, AST schema and runtime behavior
 * required for condition-controlled loops.
 *
 * Each loop contains a required expression condition and a required statement
 * body.
 */
class WhileLoopsFeature final : public EssentialFeature {
public:
    /**
     * @brief Returns metadata describing this feature.
     *
     * The feature provides the configured while-loop node kind and the
     * statement trait.
     *
     * @return Feature registration and dependency information.
     */
    EssentialInfo info() const override;

    /**
     * @brief Installs while-loop support into an Essentials controller.
     *
     * Installation registers the configured while keyword, creates the
     * while-loop AST schema and installs parser and runtime handlers.
     *
     * Runtime execution evaluates the condition before every iteration using
     * Value::truthy(), executes the body while the result remains truthy and
     * stops early when a pending return value is detected.
     *
     * When maxLoopIterations is greater than zero, exceeding that limit throws
     * a runtime error.
     *
     * @param controller Controller receiving parser, schema, and runtime registrations.
     */
    void install(EssentialsController &controller) const override;
};

/**
 * @brief Creates a pack containing while-loop support.
 *
 * The returned pack contains one WhileLoopsFeature instance.
 *
 * @return Ownable feature pack.
 */
EssentialPack whileLoops();

/**
 * @brief Creates the standard control-flow pack exposed by this namespace.
 *
 * @return Ownable feature pack.
 */
EssentialPack standard();

} // namespace novac::assets::essentials::controlflow
