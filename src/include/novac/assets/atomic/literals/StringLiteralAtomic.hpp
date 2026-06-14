#pragma once

#include "novac/assets/atomic/LiteralFeature.hpp"

#include <string>

namespace novac::assets::atomic::literals {

/**
 * @brief Registers support for string literal expressions.
 *
 * This feature installs parsing, AST node creation, and runtime
 * evaluation for string literals. Parsed literals produce an AST
 * node containing a required string field named "value" and
 * evaluate to a runtime string value.
 *
 * The accepted token format is defined by the supplied token pattern,
 * including optional suffix validation rules.
 */
class StringLiteralAtomic final : public LiteralFeature {
public:
    /**
     * @brief Creates a string literal feature.
     *
     * The node kind identifies the AST node type created for parsed
     * literals. The token pattern determines how string literals
     * are recognized and matched.
     *
     * @param nodeKind AST node kind used for generated literal nodes.
     * @param pattern Token pattern used to recognize string literals.
     *
     * @throws std::runtime_error If the node kind is empty.
     */
    explicit StringLiteralAtomic(
        std::string nodeKind = "StringLiteral",
        TokenPattern pattern = TokenPattern::key("$string"));

    /**
     * @brief Returns metadata describing this literal feature.
     *
     * @return Feature identification and registration information.
     */
    LiteralInfo info() const override;

    /**
     * @brief Installs string literal support into an atomic controller.
     *
     * Registers the configured token pattern, defines the associated
     * AST node schema, installs parsing rules, validates configured
     * suffix constraints, and registers runtime evaluation for
     * produced nodes.
     *
     * @param controller Controller receiving the feature registration.
     */
    void install(AtomicController &controller) const override;

private:
    std::string nodeKind_;
    TokenPattern pattern_;
};

} // namespace novac::assets::atomic::literals