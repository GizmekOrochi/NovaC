#pragma once

#include "novac/assets/atomic/AtomicPattern.hpp"
#include "novac/engine/EngineController.hpp"

#include <string>
#include <vector>

namespace novac::assets::atomic {

/**
 * @brief Describes the operand structure of an operation.
 *
 * The arity determines how the operation is parsed and where its operand or
 * operands appear relative to the operator token.
 */
enum class OperationArity {
    /**
     * @brief Operation accepts a single operand before evaluation.
     *
     * The operator appears before its operand.
     */
    Unary,

    /**
     * @brief Operation accepts two operands.
     *
     * One operand appears on each side of the operator.
     */
    Binary,

    /**
     * @brief Operation accepts a single operand and appears after it.
     *
     * The operator is parsed after the expression it applies to.
     */
    Postfix
};

/**
 * @brief Describes an operation feature available to an atomic controller.
 *
 * OperationInfo contains the metadata required to identify, register and
 * document an operation implementation.
 *
 * It describes how the operator is recognized, how it participates in Pratt
 * parsing, and which capabilities it provides or requires.
 */
struct OperationInfo {
    /**
     * @brief Unique identifier of the operation.
     */
    std::string id{};

    /**
     * @brief Operation version.
     */
    std::string version{"0.1.0"};

    /**
     * @brief Human-readable description of the operation.
     */
    std::string description{};

    /**
     * @brief Operand structure of the operation.
     *
     * This determines whether the operation is registered as a unary, binary,
     * or postfix parser rule.
     */
    OperationArity arity{OperationArity::Binary};

    /**
     * @brief Token pattern used to recognize the operation.
     *
     * The pattern determines how the parser binding identifies the operator.
     */
    TokenPattern pattern{};

    /**
     * @brief Parser precedence assigned to the operation.
     *
     * Precedence controls how strongly the operator binds relative to other
     * registered operations.
     */
    int precedence{};

    /**
     * @brief Associativity used when parsing chained operations.
     *
     * This mainly affects binary operators with the same precedence.
     */
    parser::Associativity associativity{parser::Associativity::Left};

    /**
     * @brief Capabilities provided by this operation.
     *
     * These capabilities can satisfy dependencies declared by other atomic
     * features.
     */
    std::vector<std::string> capabilities{};

    /**
     * @brief Capabilities required before this operation can be used.
     *
     * Installation can use this list to ensure required functionality is
     * already available.
     */
    std::vector<std::string> requiredCapabilities{};
};

class AtomicController;

/**
 * @brief Base interface for operation feature implementations.
 *
 * OperationFeature defines the common contract used by atomic operator modules.
 * Each implementation exposes its metadata through info() and installs the
 * parser and runtime behavior required by the operation through install().
 *
 * Implementations are installed through an AtomicController and are not owned
 * by the controller.
 */
class OperationFeature {
public:
    /**
     * @brief Virtual destructor.
     *
     * Allows derived operation implementations to be destroyed safely through
     * an OperationFeature pointer or reference.
     */
    virtual ~OperationFeature();

    /**
     * @brief Returns metadata describing the operation feature.
     *
     * The returned structure defines the operator pattern, arity, precedence,
     * associativity and capability requirements.
     *
     * @return Operation registration and capability information.
     */
    virtual OperationInfo info() const = 0;

    /**
     * @brief Installs the operation feature into an atomic controller.
     *
     * Implementations use the controller to register the parser bindings, AST
     * construction behavior and runtime evaluation logic required by the
     * operation.
     *
     * @param controller Controller receiving the operation registration.
     */
    virtual void install(AtomicController &controller) const = 0;
};

} // namespace novac::assets::atomic