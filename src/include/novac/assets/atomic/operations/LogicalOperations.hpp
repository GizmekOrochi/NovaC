#pragma once

#include "novac/assets/atomic/OperationFeature.hpp"

namespace novac::assets::atomic::operations {

/**
 * @brief Registers the logical AND operator.
 *
 * Logical AND is a binary left-associative operation using runtime truthiness
 * semantics.
 *
 * Evaluation short-circuits: the right operand is evaluated only when the left
 * operand is truthy.
 */
class LogicalAndOperationAtomic final : public OperationFeature {
public:
    /**
     * @brief Creates a logical AND operation.
     *
     * The operation uses precedence 4.
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
     * Installation registers a binary infix parser rule and a runtime handler
     * using the shared binary expression carrier node.
     *
     * At runtime, the left operand is evaluated first. If it is falsey, false
     * is returned immediately without evaluating the right operand.
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
 * Logical OR is a binary left-associative operation using runtime truthiness
 * semantics.
 *
 * Evaluation short-circuits: the right operand is evaluated only when the left
 * operand is falsey.
 */
class LogicalOrOperationAtomic final : public OperationFeature {
public:
    /**
     * @brief Creates a logical OR operation.
     *
     * The operation uses precedence 3.
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
     * Installation registers a binary infix parser rule and a runtime handler
     * using the shared binary expression carrier node.
     *
     * At runtime, the left operand is evaluated first. If it is truthy, true is
     * returned immediately without evaluating the right operand.
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
 * Logical NOT is a prefix unary operation that negates the runtime truthiness
 * of its operand and always produces a boolean value.
 */
class LogicalNotOperationAtomic final : public OperationFeature {
public:
    /**
     * @brief Creates a logical NOT operation.
     *
     * The operation uses precedence 30 and right associativity.
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
     * The operator is registered as a prefix parser rule. Parsing consumes the
     * operator, parses its operand at unary precedence, and creates the shared
     * unary carrier node.
     *
     * Runtime evaluation dispatches through AtomicController using the stored
     * operation id and returns the negated truthiness of the operand.
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
 * Numeric negation is a prefix unary operation supporting integer and
 * floating-point runtime values.
 *
 * Integer operands remain integers. Negating the minimum representable integer
 * raises std::runtime_error instead of overflowing. If the operand is not an
 * integer, floating-point negation is attempted instead.
 */
class NumericNegateOperationAtomic final : public OperationFeature {
public:
    /**
     * @brief Creates a numeric negation operation.
     *
     * The operation uses precedence 30 and right associativity.
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
     * The operator is registered as a prefix parser rule and uses the shared
     * unary expression carrier node.
     *
     * Runtime evaluation performs checked integer negation for integer operands.
     * If the operand is not an integer, it is converted to floating-point and
     * negated as a double.
     *
     * @param controller Controller receiving the operation registration.
     */
    void install(AtomicController &controller) const override;

private:
    TokenPattern pattern_;
};

} // namespace novac::assets::atomic::operations
