#pragma once

#include "novac/assets/atomic/LiteralFeature.hpp"

#include <string>

namespace novac::assets::atomic::literals {

/**
 * @brief Registers support for integer literal expressions.
 *
 * IntegerLiteralAtomic installs the AST schema, parser rule and runtime handler
 * required for integer literals.
 *
 * Parsed literals produce a node containing a required integer field named
 * "value". The supplied TokenPattern controls parser matching and may
 * optionally require an exact suffix or a suffix matching a regular expression.
 */
class IntegerLiteralAtomic final : public LiteralFeature {
public:
    /**
     * @brief Creates an integer literal feature.
     *
     * The node kind identifies the AST node created for parsed literals. The
     * token pattern determines the parser key and any suffix restrictions.
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
     * The feature provides the "literal.integer" and "expression.atom"
     * capabilities.
     *
     * @return Feature identification and registration information.
     */
    LiteralInfo info() const override;

    /**
     * @brief Installs integer literal support into an atomic controller.
     *
     * Installation registers the token pattern, creates an AST schema with a
     * required integer "value" field and installs a prefix parser rule.
     *
     * The parser consumes an integer token, validates exact or regex suffix
     * requirements when configured, converts the token text with std::stoi(),
     * and stores the result in the AST node.
     *
     * Runtime evaluation reads the stored integer field and returns it as a
     * runtime integer value.
     *
     * @param controller Controller receiving the feature registration.
     */
    void install(AtomicController &controller) const override;

private:
    std::string nodeKind_;
    TokenPattern pattern_;
};

} // namespace novac::assets::atomic::literals
