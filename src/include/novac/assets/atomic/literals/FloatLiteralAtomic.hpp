#pragma once

#include "novac/assets/atomic/LiteralFeature.hpp"

#include <string>

namespace novac::assets::atomic::literals {

/**
 * @brief Registers support for floating-point literal expressions.
 *
 * This feature installs parsing, AST node creation, and runtime
 * evaluation for floating-point literals. Parsed literals produce
 * an AST node containing a required floating-point field named
 * "value" and evaluate to a runtime floating-point value.
 *
 * The accepted token format is defined by the supplied token pattern,
 * including optional suffix validation rules.
 */
class FloatLiteralAtomic final : public LiteralFeature {
public:
    /**
     * @brief Creates a floating-point literal feature.
     *
     * The node kind identifies the AST node type created for parsed
     * literals. The token pattern determines how floating-point
     * literals are recognized and matched.
     *
     * @param nodeKind AST node kind used for generated literal nodes.
     * @param pattern Token pattern used to recognize floating-point literals.
     *
     * @throws std::runtime_error If the node kind is empty.
     */
    explicit FloatLiteralAtomic(std::string nodeKind = "FloatLiteral", TokenPattern pattern = TokenPattern::key("$float"));

    /**
     * @brief Returns metadata describing this literal feature.
     *
     * @return Feature identification and registration information.
     */
    LiteralInfo info() const override;

    /**
     * @brief Installs floating-point literal support into an atomic controller.
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