#pragma once

#include "novac/assets/atomic/OperationFeature.hpp"

namespace novac::assets::atomic::operations {

/**
 * @brief Registers the logical AND operator.
 *
 * Produces a binary logical expression that evaluates operands
 * using truthiness semantics and returns a boolean result.
 *
 * Runtime evaluation uses short-circuit behavior: the right operand
 * is evaluated only when the left operand is truthy.
 */
class LogicalAndOperationAtomic final : public OperationFeature {
public:
    /**
     * @brief Creates a logical AND operation.
     *
     * @param pattern Token pattern used to recognize the operator.
     */
    explicit LogicalAndOperationAtomic(
        TokenPattern pattern = TokenPattern::text("&&"));

    /**
     * @brief Returns metadata describing this operation.
     *
     * @return Operation registration information.
     */
    OperationInfo info() const override;

    /**
     * @brief Installs the logical AND operator into an atomic controller.
     *
     * Registers parsing rules, AST construction logic, and runtime
     * evaluation behavior for logical conjunction.
     *
     * @param controller Controller receiving the operation registration.
     */
    void install(AtomicController &controller) const override;

private:
    TokenPattern pattern_;
};

/**
 * @brief Registers the logical OR operator.
 *
 * Produces a binary logical expression that evaluates operands
 * using truthiness semantics and returns a boolean result.
 *
 * Runtime evaluation uses short-circuit behavior: the right operand
 * is evaluated only when the left operand is not truthy.
 */
class LogicalOrOperationAtomic final : public OperationFeature {
public:
    /**
     * @brief Creates a logical OR operation.
     *
     * @param pattern Token pattern used to recognize the operator.
     */
    explicit LogicalOrOperationAtomic(
        TokenPattern pattern = TokenPattern::text("||"));

    /**
     * @brief Returns metadata describing this operation.
     *
     * @return Operation registration information.
     */
    OperationInfo info() const override;

    /**
     * @brief Installs the logical OR operator into an atomic controller.
     *
     * Registers parsing rules, AST construction logic, and runtime
     * evaluation behavior for logical disjunction.
     *
     * @param controller Controller receiving the operation registration.
     */
    void install(AtomicController &controller) const override;

private:
    TokenPattern pattern_;
};

/**
 * @brief Registers the logical NOT operator.
 *
 * Produces a unary logical expression that negates the truthiness
 * of its operand and returns a boolean result.
 */
class LogicalNotOperationAtomic final : public OperationFeature {
public:
    /**
     * @brief Creates a logical NOT operation.
     *
     * @param pattern Token pattern used to recognize the operator.
     */
    explicit LogicalNotOperationAtomic(
        TokenPattern pattern = TokenPattern::text("!"));

    /**
     * @brief Returns metadata describing this operation.
     *
     * @return Operation registration information.
     */
    OperationInfo info() const override;

    /**
     * @brief Installs the logical NOT operator into an atomic controller.
     *
     * Registers parsing rules, AST construction logic, and runtime
     * evaluation behavior for logical negation.
     *
     * @param controller Controller receiving the operation registration.
     */
    void install(AtomicController &controller) const override;

private:
    TokenPattern pattern_;
};

/**
 * @brief Registers the unary numeric negation operator.
 *
 * Produces a unary expression that negates integer or floating-point
 * values and returns a value of the corresponding numeric type.
 */
class NumericNegateOperationAtomic final : public OperationFeature {
public:
    /**
     * @brief Creates a numeric negation operation.
     *
     * @param pattern Token pattern used to recognize the operator.
     */
    explicit NumericNegateOperationAtomic(
        TokenPattern pattern = TokenPattern::text("-"));

    /**
     * @brief Returns metadata describing this operation.
     *
     * @return Operation registration information.
     */
    OperationInfo info() const override;

    /**
     * @brief Installs the numeric negation operator into an atomic controller.
     *
     * Registers parsing rules, AST construction logic, and runtime
     * evaluation behavior for unary numeric negation.
     *
     * @param controller Controller receiving the operation registration.
     */
    void install(AtomicController &controller) const override;

private:
    TokenPattern pattern_;
};

} // namespace novac::assets::atomic::operations