#pragma once

#include "novac/assets/atomic/AtomicPattern.hpp"

#include <string>
#include <vector>

namespace novac::assets::atomic {

class AtomicController;

/**
 * @brief Describes a literal feature available to an atomic controller.
 *
 * Provides metadata used for registration, discovery, compatibility
 * checks, and documentation of literal implementations.
 */
struct LiteralInfo {
    /**
     * @brief Unique identifier of the literal feature.
     */
    std::string id{};

    /**
     * @brief Feature version.
     */
    std::string version{"0.1.0"};

    /**
     * @brief Human-readable description of the literal.
     */
    std::string description{};

    /**
     * @brief AST node kind produced by the literal.
     */
    std::string nodeKind{};

    /**
     * @brief Token pattern used to recognize the literal.
     */
    TokenPattern pattern{};

    /**
     * @brief Capabilities provided by this literal.
     */
    std::vector<std::string> capabilities{};

    /**
     * @brief Capabilities required before this literal can be used.
     */
    std::vector<std::string> requiredCapabilities{};
};

/**
 * @brief Base interface for literal feature implementations.
 *
 * Literal features register parsing rules, AST node generation,
 * and runtime evaluation behavior for literal expressions.
 *
 * Implementations are installed through an AtomicController and are
 * not owned by the controller.
 */
class LiteralFeature {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~LiteralFeature();

    /**
     * @brief Returns metadata describing the literal feature.
     *
     * @return Literal registration and capability information.
     */
    virtual LiteralInfo info() const = 0;

    /**
     * @brief Installs the literal feature into an atomic controller.
     *
     * Implementations register any required parser bindings, AST node
     * definitions, and runtime evaluation handlers.
     *
     * @param controller Controller receiving the feature registration.
     */
    virtual void install(AtomicController &controller) const = 0;
};

} // namespace novac::assets::atomic