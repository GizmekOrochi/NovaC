#pragma once

#include "novac/assets/atomic/LiteralFeature.hpp"

#include <string>

namespace novac::assets::atomic::literals {

/**
 * @brief Registers support for integer literal expressions.
 *
 * This feature installs parsing, AST node creation, and runtime
 * evaluation for integer literals. Parsed literals produce an AST
 * node containing a required integer field named "value" and
 * evaluate to a runtime integer value.
 *
 * The accepted token format is defined by the supplied token pattern,
 * including optional suffix validation rules.
 */
class IntegerLiteralAtomic final : public LiteralFeature {
public:
    /**
     * @brief Creates an integer literal feature.
     *
     * The node kind identifies the AST node type created for parsed
     * literals. The token pattern determines how integer literals
     * are recognized and matched.
     *
     * @param nodeKind AST node kind used for generated literal nodes.
     * @param pattern Token pattern used to recognize integer literals.
     *
     * @throws std::runtime_error If the node kind is empty.
     */
    explicit IntegerLiteralAtomic(std::string nodeKind = "IntegerLiteral", TokenPattern pattern = TokenPattern::key("$int"));

    /**
     * @brief Returns metadata describing this literal feature.
     *
     * @return Feature identification and registration information.
     */
    LiteralInfo info() const override;

    /**
     * @brief Installs integer literal support into an atomic controller.
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