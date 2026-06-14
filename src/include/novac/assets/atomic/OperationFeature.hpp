#pragma once

#include "novac/assets/atomic/AtomicPattern.hpp"
#include "novac/engine/EngineController.hpp"

#include <string>
#include <vector>

namespace novac::assets::atomic {

/**
 * @brief Describes the operand structure of an operation.
 */
enum class OperationArity {
    /**
     * @brief Operation accepts a single operand before evaluation.
     */
    Unary,

    /**
     * @brief Operation accepts two operands.
     */
    Binary,

    /**
     * @brief Operation accepts a single operand and appears after it.
     */
    Postfix
};

/**
 * @brief Describes an operation feature available to an atomic controller.
 *
 * Provides metadata used for registration, parser integration,
 * capability discovery, and documentation.
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
     */
    OperationArity arity{OperationArity::Binary};

    /**
     * @brief Token pattern used to recognize the operation.
     */
    TokenPattern pattern{};

    /**
     * @brief Parser precedence assigned to the operation.
     */
    int precedence{};

    /**
     * @brief Associativity used when parsing chained operations.
     */
    parser::Associativity associativity{parser::Associativity::Left};

    /**
     * @brief Capabilities provided by this operation.
     */
    std::vector<std::string> capabilities{};

    /**
     * @brief Capabilities required before this operation can be used.
     */
    std::vector<std::string> requiredCapabilities{};
};

class AtomicController;

/**
 * @brief Base interface for operation feature implementations.
 *
 * Operation features register parser rules, AST construction behavior,
 * and runtime evaluation logic for unary, binary, or postfix operators.
 *
 * Implementations are installed through an AtomicController and are
 * not owned by the controller.
 */
class OperationFeature {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~OperationFeature();

    /**
     * @brief Returns metadata describing the operation feature.
     *
     * @return Operation registration and capability information.
     */
    virtual OperationInfo info() const = 0;

    /**
     * @brief Installs the operation feature into an atomic controller.
     *
     * Implementations register any required parser bindings, AST node
     * construction rules, and runtime evaluation handlers.
     *
     * @param controller Controller receiving the operation registration.
     */
    virtual void install(AtomicController &controller) const = 0;
};

} // namespace novac::assets::atomic