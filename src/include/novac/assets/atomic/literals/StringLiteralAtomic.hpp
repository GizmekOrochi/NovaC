#pragma once

#include "novac/assets/atomic/LiteralFeature.hpp"

#include <string>

namespace novac::assets::atomic::literals {

/**
 * @brief Registers support for string literal expressions.
 *
 * StringLiteralAtomic installs the AST schema, parser rule and runtime handler
 * required for string literals.
 *
 * Parsed literals produce a node containing a required string field named
 * "value". The supplied TokenPattern controls parser matching and may
 * optionally require an exact suffix or a suffix matching a regular expression.
 */
class StringLiteralAtomic final : public LiteralFeature {
public:
    /**
     * @brief Creates a string literal feature.
     *
     * The node kind identifies the AST node created for parsed literals. The
     * token pattern determines the parser key and any suffix restrictions.
     *
     * @param nodeKind AST node kind used for generated literal nodes.
     * @param pattern Token pattern used to recognize string literals.
     *
     * @throws std::runtime_error If the node kind is empty.
     */
    explicit StringLiteralAtomic(std::string nodeKind = "StringLiteral", TokenPattern pattern = TokenPattern::key("$string"));

    /**
     * @brief Returns metadata describing this literal feature.
     *
     * The feature provides the "literal.string" and "expression.atom"
     * capabilities.
     *
     * @return Feature identification and registration information.
     */
    LiteralInfo info() const override;

    /**
     * @brief Installs string literal support into an atomic controller.
     *
     * Installation registers the token pattern, creates an AST schema with a
     * required string "value" field and installs a prefix parser rule.
     *
     * The parser consumes a string token, validates exact or regex suffix
     * requirements when configured, and stores the token text directly in the
     * AST node.
     *
     * Runtime evaluation reads the stored field and returns it as a runtime
     * string value.
     *
     * @param controller Controller receiving the feature registration.
     */
    void install(AtomicController &controller) const override;

private:
    std::string nodeKind_;
    TokenPattern pattern_;
};

} // namespace novac::assets::atomic::literals
