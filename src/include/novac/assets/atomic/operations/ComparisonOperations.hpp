#pragma once

#include "novac/assets/atomic/OperationFeature.hpp"

#include <string>

namespace novac::assets::atomic::operations {

/**
 * @brief Base class for numeric comparison operations.
 *
 * NumericComparisonOperationAtomic contains the common parser and runtime
 * registration logic shared by numeric comparison operators.
 *
 * Installation creates a binary parser binding using the shared binary carrier
 * node. At runtime, both operands are evaluated and converted to floating-point
 * values before compare() is called.
 *
 * Derived classes only define the concrete comparison semantics.
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
     * Numeric comparisons use precedence 7, are non-associative and provide
     * the "operation.comparison" and "operation.binary" capabilities.
     *
     * @return Operation registration information.
     */
    OperationInfo info() const override;

    /**
     * @brief Installs the comparison operator into an atomic controller.
     *
     * The token pattern and binary carrier node are registered first. An infix
     * parser rule then creates a binary node containing the operation id and
     * both operand nodes.
     *
     * Runtime evaluation converts both operands with Value::asFloat(), invokes
     * compare(), and wraps the resulting boolean in a runtime Value.
     *
     * @param controller Controller receiving the operation registration.
     */
    void install(AtomicController &controller) const override;

protected:
    /**
     * @brief Compares two numeric values.
     *
     * Derived classes implement only the final comparison performed after both
     * runtime operands have been converted to double.
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
 * Returns true when both numeric operands compare equal after conversion to
 * floating-point values.
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
 * Returns true when both numeric operands compare different after conversion
 * to floating-point values.
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
 * Evaluates whether the left numeric operand is smaller than the right one.
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
 * Evaluates whether the left numeric operand is less than or equal to the
 * right one.
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
 * Evaluates whether the left numeric operand is greater than the right one.
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
 * Evaluates whether the left numeric operand is greater than or equal to the
 * right one.
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
