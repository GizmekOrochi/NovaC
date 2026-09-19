#pragma once

#include "novac/assets/atomic/OperationFeature.hpp"

#include <functional>
#include <string>

namespace novac::assets::atomic::operations {

/**
 * @brief Base class for binary numeric operations.
 *
 * NumericBinaryOperationAtomic contains the common parser and runtime
 * registration logic shared by arithmetic operators.
 *
 * Installation registers the token pattern, ensures the shared binary
 * expression carrier node exists, installs an infix parser rule and connects
 * the operation id to a runtime binary handler.
 *
 * Derived classes only need to create an independent evaluator defining the
 * actual arithmetic behavior.
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
     * Numeric binary operations are left-associative and provide the
     * "operation.numeric" and "operation.binary" capabilities.
     *
     * @return Operation registration information.
     */
    OperationInfo info() const override;

    /**
     * @brief Installs the arithmetic operator into an atomic controller.
     *
     * The operator pattern is first registered with the controller and the
     * shared binary expression node is ensured.
     *
     * An infix parser rule then creates a carrier node containing the operation
     * id and both operands. Runtime evaluation evaluates the two child nodes
     * before forwarding their values to an evaluator copied into the runtime
 * callback. The callback therefore remains valid after this feature object has
 * been destroyed.
     *
     * @param controller Controller receiving the operation registration.
     */
    void install(AtomicController &controller) const override;

protected:
    using Evaluator = std::function<runtime::Value(
        const runtime::Value &left,
        const runtime::Value &right)>;

    /**
     * @brief Creates an independent runtime evaluator.
     *
     * The returned callable is copied into the engine runtime callback and must
     * not depend on the lifetime of this OperationFeature instance. Any state
     * required by a derived operation should be captured by value.
     *
     * @return Callable implementing the arithmetic operation.
     */
    virtual Evaluator makeEvaluator() const = 0;

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
 *
 * Integer results are preserved when both operands are integers. Integer
 * overflow raises std::runtime_error instead of wrapping. Otherwise, both
 * operands are evaluated as floating-point values.
 */
class AddOperationAtomic final : public NumericBinaryOperationAtomic {
public:
    /**
     * @brief Creates an addition operation.
     *
     * Addition uses precedence 10.
     *
     * @param pattern Token pattern used to recognize the operator.
     */
    explicit AddOperationAtomic(
        TokenPattern pattern = TokenPattern::text("+"));

private:
    Evaluator makeEvaluator() const override;
};

/**
 * @brief Numeric subtraction operation.
 *
 * Computes the difference between two numeric operands.
 *
 * Integer results are preserved when both operands are integers. Integer
 * overflow raises std::runtime_error instead of wrapping. Otherwise,
 * floating-point arithmetic is used.
 */
class SubtractOperationAtomic final : public NumericBinaryOperationAtomic {
public:
    /**
     * @brief Creates a subtraction operation.
     *
     * Subtraction uses precedence 10.
     *
     * @param pattern Token pattern used to recognize the operator.
     */
    explicit SubtractOperationAtomic(
        TokenPattern pattern = TokenPattern::text("-"));

private:
    Evaluator makeEvaluator() const override;
};

/**
 * @brief Numeric multiplication operation.
 *
 * Computes the product of two numeric operands.
 *
 * Integer results are preserved when both operands are integers. Integer
 * overflow raises std::runtime_error instead of wrapping. Otherwise,
 * floating-point arithmetic is used.
 */
class MultiplyOperationAtomic final : public NumericBinaryOperationAtomic {
public:
    /**
     * @brief Creates a multiplication operation.
     *
     * Multiplication uses precedence 20.
     *
     * @param pattern Token pattern used to recognize the operator.
     */
    explicit MultiplyOperationAtomic(
        TokenPattern pattern = TokenPattern::text("*"));

private:
    Evaluator makeEvaluator() const override;
};

/**
 * @brief Numeric division operation.
 *
 * Computes the quotient of two numeric operands.
 *
 * When both operands are integers, checked integer division semantics are
 * preserved. Division by zero and an out-of-range quotient raise
 * std::runtime_error. Otherwise, floating-point division is used.
 */
class DivideOperationAtomic final : public NumericBinaryOperationAtomic {
public:
    /**
     * @brief Creates a division operation.
     *
     * Division uses precedence 20.
     *
     * @param pattern Token pattern used to recognize the operator.
     */
    explicit DivideOperationAtomic(
        TokenPattern pattern = TokenPattern::text("/"));

private:
    /**
     * @brief Evaluates a division operation.
     *
     * The divisor is checked before either integer or floating-point division
     * is performed.
     *
     * @param left Left operand value.
     * @param right Right operand value.
     * @return Quotient of the operands.
     *
     * @throws std::runtime_error If the divisor is zero or the integer quotient
     * cannot be represented by the runtime int type.
     */
    Evaluator makeEvaluator() const override;
};

/**
 * @brief Numeric modulo operation.
 *
 * Computes the remainder of a division operation.
 *
 * Integer operands use checked remainder semantics. `INT_MIN % -1` is defined
 * as zero without evaluating the undefined C++ expression. If either operand
 * is floating-point, std::fmod semantics are used instead.
 */
class ModuloOperationAtomic final : public NumericBinaryOperationAtomic {
public:
    /**
     * @brief Creates a modulo operation.
     *
     * Modulo uses precedence 20.
     *
     * @param pattern Token pattern used to recognize the operator.
     */
    explicit ModuloOperationAtomic(
        TokenPattern pattern = TokenPattern::text("%"));

private:
    /**
     * @brief Evaluates a modulo operation.
     *
     * The divisor is checked before either integer modulo or floating-point
     * remainder evaluation is performed.
     *
     * @param left Left operand value.
     * @param right Right operand value.
     * @return Remainder of the division.
     *
     * @throws std::runtime_error If the divisor is zero.
     */
    Evaluator makeEvaluator() const override;
};

} // namespace novac::assets::atomic::operations
