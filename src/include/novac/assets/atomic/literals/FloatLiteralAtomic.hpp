#pragma once

#include "novac/assets/atomic/LiteralFeature.hpp"

#include <string>

namespace novac::assets::atomic::literals {

/**
 * @brief Registers support for floating-point literal expressions.
 *
 * FloatLiteralAtomic installs the AST schema, parser rule and runtime handler
 * required for floating-point literals.
 *
 * Parsed literals produce a node containing a required floating-point field
 * named "value". The supplied TokenPattern controls parser matching and may
 * optionally require an exact suffix or a suffix matching a regular expression.
 */
class FloatLiteralAtomic final : public LiteralFeature {
public:
    /**
     * @brief Creates a floating-point literal feature.
     *
     * The node kind identifies the AST node created for parsed literals. The
     * token pattern determines the parser key and any suffix restrictions.
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
     * The feature provides the "literal.float" and "expression.atom"
     * capabilities.
     *
     * @return Feature identification and registration information.
     */
    LiteralInfo info() const override;

    /**
     * @brief Installs floating-point literal support into an atomic controller.
     *
     * Installation registers the token pattern, creates an AST schema with a
     * required floating-point "value" field and installs a prefix parser rule.
     *
     * The parser consumes a floating-point token, validates exact or regex
     * suffix requirements when configured, converts the token text with
     * std::stod(), and stores the result in the AST node.
     *
     * Runtime evaluation reads the stored field and returns it as a floating
     * runtime value.
     *
     * @param controller Controller receiving the feature registration.
     */
    void install(AtomicController &controller) const override;

private:
    std::string nodeKind_;
    TokenPattern pattern_;
};

} // namespace novac::assets::atomic::literals
