#pragma once

#include "novac/assets/atomic/AtomicPattern.hpp"

#include <string>
#include <vector>

namespace novac::assets::atomic {

class AtomicController;

/**
 * @brief Describes a literal feature available to an atomic controller.
 *
 * LiteralInfo contains the metadata needed to identify, document and validate
 * a literal implementation before or during installation.
 *
 * The information includes the AST node kind produced by the literal, the token
 * pattern used to recognize it, and its capability dependencies.
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
     *
     * Literal parsing implementations generally create nodes using this kind.
     */
    std::string nodeKind{};

    /**
     * @brief Token pattern used to recognize the literal.
     *
     * The pattern describes which token form should be associated with this
     * literal implementation.
     */
    TokenPattern pattern{};

    /**
     * @brief Capabilities provided by this literal.
     *
     * These capabilities can be used by other atomic features when checking
     * dependencies.
     */
    std::vector<std::string> capabilities{};

    /**
     * @brief Capabilities required before this literal can be used.
     *
     * Installation can use this list to ensure that required functionality is
     * already available.
     */
    std::vector<std::string> requiredCapabilities{};
};

/**
 * @brief Base interface for literal feature implementations.
 *
 * LiteralFeature defines the common contract used by atomic literal modules.
 * Each implementation provides its own metadata through info() and installs
 * its parser, AST and runtime behavior through install().
 *
 * Implementations are installed through an AtomicController and are not owned
 * by the controller.
 */
class LiteralFeature {
public:
    /**
     * @brief Virtual destructor.
     *
     * Allows derived literal implementations to be destroyed safely through
     * a LiteralFeature pointer or reference.
     */
    virtual ~LiteralFeature();

    /**
     * @brief Returns metadata describing the literal feature.
     *
     * The returned structure identifies the literal and describes its node
     * kind, token pattern and capability requirements.
     *
     * @return Literal registration and capability information.
     */
    virtual LiteralInfo info() const = 0;

    /**
     * @brief Installs the literal feature into an atomic controller.
     *
     * Implementations use the controller to register the parser bindings,
     * AST schema and runtime behavior required by the literal.
     *
     * @param controller Controller receiving the feature registration.
     */
    virtual void install(AtomicController &controller) const = 0;
};

} // namespace novac::assets::atomic