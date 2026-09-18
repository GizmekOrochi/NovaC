#pragma once

#include "novac/assets/essentials/EssentialFeature.hpp"

namespace novac::assets::essentials::scopes {

/**
 * @brief Installs lexically scoped statement blocks.
 *
 * ScopedBlocksFeature registers brace-delimited blocks as statement nodes and
 * gives them their own runtime lexical scope.
 *
 * Parsed blocks contain an ordered list of child statements. At runtime, the
 * feature pushes a new environment before executing those statements and
 * restores the previous scope afterwards.
 */
class ScopedBlocksFeature final : public EssentialFeature {
public:
    /**
     * @brief Returns metadata describing this feature.
     *
     * The feature provides the configured block statement node together with
     * the statement and scope traits.
     *
     * @return Feature registration and dependency information.
     */
    EssentialInfo info() const override;

    /**
     * @brief Installs scoped-block support into an Essentials controller.
     *
     * Installation registers the configured opening and closing brace tokens,
     * creates the block AST schema and installs parser and runtime handlers.
     *
     * Parsing collects statements until the closing brace is reached. Reaching
     * the end of the token stream first is treated as an unterminated block.
     *
     * Runtime execution creates a nested scope, executes statements in source
     * order and stops early when a pending return value is detected. The scope
     * is removed both after normal execution and when an exception is thrown.
     *
     * @param controller Controller receiving parser, schema, and runtime registrations.
     */
    void install(EssentialsController &controller) const override;
};

/**
 * @brief Creates a pack containing scoped-block support.
 *
 * The returned pack contains one ScopedBlocksFeature instance.
 *
 * @return Ownable feature pack.
 */
EssentialPack scopedBlocks();

/**
 * @brief Creates the standard scope feature pack.
 *
 * The standard scope pack currently contains scoped-block support.
 *
 * @return Ownable feature pack.
 */
EssentialPack standard();

} // namespace novac::assets::essentials::scopes
