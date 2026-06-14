#pragma once

#include "novac/assets/atomic/OperationFeature.hpp"

#include <string>

namespace novac::assets::atomic::operations {

/**
 * @brief Base class for binary numeric operations.
 *
 * Registers a binary arithmetic operator that evaluates two operands
 * and produces a numeric result. Implementations define the actual
 * arithmetic behavior through evaluate().
 *
 * When both operands are integers, operations may preserve integer
 * semantics. Otherwise, evaluation is performed using floating-point
 * values.
 */
class NumericBinaryOperationAtomic : public OperationFeature {
public:
    /**
     * @brief Creates a binary numeric operation.
     *
     * @param id Unique operation identifier.
     * @param description Human-readable operation description.
     * @param pattern Token pattern used to recognize the operator.
     * @param precedence Operator precedence used by the parser.
     *
     * @throws std::runtime_error If the operation identifier is empty.
     */
    NumericBinaryOperationAtomic(
        std::string id,
        std::string description,
        TokenPattern pattern,
        int precedence);

    /**
     * @brief Returns metadata describing this operation.
     *
     * @return Operation registration information.
     */
    OperationInfo info() const override;

    /**
     * @brief Installs the arithmetic operator into an atomic controller.
     *
     * Registers the operator token pattern, parser rules, AST
     * construction logic, and runtime evaluation behavior.
     *
     * @param controller Controller receiving the operation registration.
     */
    void install(AtomicController &controller) const override;

protected:
    /**
     * @brief Evaluates the operation for two operand values.
     *
     * @param left Left operand value.
     * @param right Right operand value.
     * @return Result of the arithmetic operation.
     */
    virtual runtime::Value evaluate(
        const runtime::Value &left,
        const runtime::Value &right) const = 0;

private:
    std::string id_;
    std::string description_;
    TokenPattern pattern_;
    int precedence_;
};

/**
 * @brief Numeric addition operation.
 *
 * Computes the sum of two numeric operands.
 */
class AddOperationAtomic final : public NumericBinaryOperationAtomic {
public:
    /**
     * @brief Creates an addition operation.
     *
     * @param pattern Token pattern used to recognize the operator.
     */
    explicit AddOperationAtomic(
        TokenPattern pattern = TokenPattern::text("+"));

private:
    runtime::Value evaluate(
        const runtime::Value &left,
        const runtime::Value &right) const override;
};

/**
 * @brief Numeric subtraction operation.
 *
 * Computes the difference between two numeric operands.
 */
class SubtractOperationAtomic final : public NumericBinaryOperationAtomic {
public:
    /**
     * @brief Creates a subtraction operation.
     *
     * @param pattern Token pattern used to recognize the operator.
     */
    explicit SubtractOperationAtomic(
        TokenPattern pattern = TokenPattern::text("-"));

private:
    runtime::Value evaluate(
        const runtime::Value &left,
        const runtime::Value &right) const override;
};

/**
 * @brief Numeric multiplication operation.
 *
 * Computes the product of two numeric operands.
 */
class MultiplyOperationAtomic final : public NumericBinaryOperationAtomic {
public:
    /**
     * @brief Creates a multiplication operation.
     *
     * @param pattern Token pattern used to recognize the operator.
     */
    explicit MultiplyOperationAtomic(
        TokenPattern pattern = TokenPattern::text("*"));

private:
    runtime::Value evaluate(
        const runtime::Value &left,
        const runtime::Value &right) const override;
};

/**
 * @brief Numeric division operation.
 *
 * Computes the quotient of two numeric operands.
 *
 * Integer operands use integer division semantics.
 */
class DivideOperationAtomic final : public NumericBinaryOperationAtomic {
public:
    /**
     * @brief Creates a division operation.
     *
     * @param pattern Token pattern used to recognize the operator.
     */
    explicit DivideOperationAtomic(
        TokenPattern pattern = TokenPattern::text("/"));

private:
    /**
     * @brief Evaluates a division operation.
     *
     * @param left Left operand value.
     * @param right Right operand value.
     * @return Quotient of the operands.
     *
     * @throws std::runtime_error If the divisor is zero.
     */
    runtime::Value evaluate(
        const runtime::Value &left,
        const runtime::Value &right) const override;
};

/**
 * @brief Numeric modulo operation.
 *
 * Computes the remainder of a division operation.
 *
 * Integer operands use the integer modulo operator, while
 * floating-point operands use floating-point remainder semantics.
 */
class ModuloOperationAtomic final : public NumericBinaryOperationAtomic {
public:
    /**
     * @brief Creates a modulo operation.
     *
     * @param pattern Token pattern used to recognize the operator.
     */
    explicit ModuloOperationAtomic(
        TokenPattern pattern = TokenPattern::text("%"));

private:
    /**
     * @brief Evaluates a modulo operation.
     *
     * @param left Left operand value.
     * @param right Right operand value.
     * @return Remainder of the division.
     *
     * @throws std::runtime_error If the divisor is zero.
     */
    runtime::Value evaluate(
        const runtime::Value &left,
        const runtime::Value &right) const override;
};

} // namespace novac::assets::atomic::operations