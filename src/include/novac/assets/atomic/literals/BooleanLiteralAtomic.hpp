#pragma once

#include "novac/assets/atomic/LiteralFeature.hpp"

#include <string>

namespace novac::assets::atomic::literals {

/**
 * @brief Token texts used to represent boolean literals.
 *
 * Defines the keyword strings recognized as the boolean
 * true and false literal values during parsing.
 */
struct BooleanLiteralTokens {
    std::string trueToken{"true"};
    std::string falseToken{"false"};
};

/**
 * @brief Registers support for boolean literal expressions.
 *
 * This feature installs parsing, AST node creation, and runtime
 * evaluation for boolean literals. Parsed literals produce an AST
 * node containing a required boolean field named "value" and
 * evaluate to a runtime boolean value.
 */
class BooleanLiteralAtomic final : public LiteralFeature {
public:
    /**
     * @brief Creates a boolean literal feature.
     *
     * The node kind identifies the AST node type created for boolean
     * literals. Both boolean token strings must be non-empty.
     *
     * @param nodeKind AST node kind used for generated literal nodes.
     * @param tokens Token texts recognized as boolean literals.
     *
     * @throws std::runtime_error If the node kind is empty.
     * @throws std::runtime_error If either boolean token is empty.
     */
    explicit BooleanLiteralAtomic(std::string nodeKind = "BooleanLiteral", BooleanLiteralTokens tokens = {});

    /**
     * @brief Returns metadata describing this literal feature.
     *
     * @return Feature identification and registration information.
     */
    LiteralInfo info() const override;

    /**
     * @brief Installs boolean literal support into an atomic controller.
     *
     * Registers the configured boolean tokens, defines the associated
     * AST node schema, installs parsing rules, and registers runtime
     * evaluation for produced nodes.
     *
     * @param controller Controller receiving the feature registration.
     */
    void install(AtomicController &controller) const override;

private:
    std::string nodeKind_;
    BooleanLiteralTokens tokens_;
};

} // namespace novac::assets::atomic::literals