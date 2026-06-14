#pragma once

#include "novac/assets/atomic/OperationFeature.hpp"

#include <string>

namespace novac::assets::atomic::operations {

/**
 * @brief Base class for numeric comparison operations.
 *
 * Registers a binary comparison operator that evaluates both operands
 * as floating-point values and produces a boolean result.
 *
 * Derived classes define the specific comparison semantics through
 * the compare() function.
 */
class NumericComparisonOperationAtomic : public OperationFeature {
public:
    /**
     * @brief Creates a numeric comparison operation.
     *
     * @param id Unique operation identifier.
     * @param description Human-readable operation description.
     * @param pattern Token pattern used to recognize the operator.
     *
     * @throws std::runtime_error If the operation identifier is empty.
     */
    NumericComparisonOperationAtomic(
        std::string id,
        std::string description,
        TokenPattern pattern);

    /**
     * @brief Returns metadata describing this operation.
     *
     * @return Operation registration information.
     */
    OperationInfo info() const override;

    /**
     * @brief Installs the comparison operator into an atomic controller.
     *
     * Registers the operator token pattern, parser rules, AST
     * construction logic, and runtime evaluation behavior.
     *
     * @param controller Controller receiving the operation registration.
     */
    void install(AtomicController &controller) const override;

protected:
    /**
     * @brief Compares two numeric values.
     *
     * @param left Left operand value.
     * @param right Right operand value.
     * @return Result of the comparison.
     */
    virtual bool compare(double left, double right) const = 0;

private:
    std::string id_;
    std::string description_;
    TokenPattern pattern_;
};

/**
 * @brief Numeric equality comparison operation.
 *
 * Evaluates whether two numeric operands are equal.
 */
class EqualOperationAtomic final : public NumericComparisonOperationAtomic {
public:
    /**
     * @brief Creates an equality comparison operation.
     *
     * @param pattern Token pattern used to recognize the operator.
     */
    explicit EqualOperationAtomic(TokenPattern pattern = TokenPattern::text("=="));
private:
    bool compare(double left, double right) const override;
};

/**
 * @brief Numeric inequality comparison operation.
 *
 * Evaluates whether two numeric operands are different.
 */
class NotEqualOperationAtomic final : public NumericComparisonOperationAtomic {
public:
    /**
     * @brief Creates an inequality comparison operation.
     *
     * @param pattern Token pattern used to recognize the operator.
     */
    explicit NotEqualOperationAtomic(TokenPattern pattern = TokenPattern::text("!="));
private:
    bool compare(double left, double right) const override;
};

/**
 * @brief Numeric less-than comparison operation.
 *
 * Evaluates whether the left operand is smaller than the right operand.
 */
class LessOperationAtomic final : public NumericComparisonOperationAtomic {
public:
    /**
     * @brief Creates a less-than comparison operation.
     *
     * @param pattern Token pattern used to recognize the operator.
     */
    explicit LessOperationAtomic(TokenPattern pattern = TokenPattern::text("<"));
private:
    bool compare(double left, double right) const override;
};

/**
 * @brief Numeric less-than-or-equal comparison operation.
 *
 * Evaluates whether the left operand is less than or equal to the
 * right operand.
 */
class LessEqualOperationAtomic final : public NumericComparisonOperationAtomic {
public:
    /**
     * @brief Creates a less-than-or-equal comparison operation.
     *
     * @param pattern Token pattern used to recognize the operator.
     */
    explicit LessEqualOperationAtomic(TokenPattern pattern = TokenPattern::text("<="));
private:
    bool compare(double left, double right) const override;
};

/**
 * @brief Numeric greater-than comparison operation.
 *
 * Evaluates whether the left operand is greater than the right operand.
 */
class GreaterOperationAtomic final : public NumericComparisonOperationAtomic {
public:
    /**
     * @brief Creates a greater-than comparison operation.
     *
     * @param pattern Token pattern used to recognize the operator.
     */
    explicit GreaterOperationAtomic(TokenPattern pattern = TokenPattern::text(">"));
private:
    bool compare(double left, double right) const override;
};

/**
 * @brief Numeric greater-than-or-equal comparison operation.
 *
 * Evaluates whether the left operand is greater than or equal to the
 * right operand.
 */
class GreaterEqualOperationAtomic final : public NumericComparisonOperationAtomic {
public:
    /**
     * @brief Creates a greater-than-or-equal comparison operation.
     *
     * @param pattern Token pattern used to recognize the operator.
     */
    explicit GreaterEqualOperationAtomic(TokenPattern pattern = TokenPattern::text(">="));
private:
    bool compare(double left, double right) const override;
};

} // namespace novac::assets::atomic::operations