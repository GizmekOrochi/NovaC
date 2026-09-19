#pragma once

#include <limits>
#include <stdexcept>

namespace novac::assets::atomic::operations::detail {

/**
 * @brief Adds two integers and rejects results outside the runtime int range.
 *
 * NovaC integer arithmetic is checked: signed overflow never wraps and never
 * relies on C++ undefined behavior. An out-of-range mathematical result raises
 * std::runtime_error.
 */
inline int checkedAdd(int left, int right) {
    constexpr int min{std::numeric_limits<int>::min()};
    constexpr int max{std::numeric_limits<int>::max()};

    if ((right > 0 && left > max - right) ||
        (right < 0 && left < min - right)) {
        throw std::runtime_error("integer overflow in addition");
    }

    return left + right;
}

/** @brief Subtracts two integers with checked overflow semantics. */
inline int checkedSubtract(int left, int right) {
    constexpr int min{std::numeric_limits<int>::min()};
    constexpr int max{std::numeric_limits<int>::max()};

    if ((right < 0 && left > max + right) ||
        (right > 0 && left < min + right)) {
        throw std::runtime_error("integer overflow in subtraction");
    }

    return left - right;
}

/** @brief Multiplies two integers with checked overflow semantics. */
inline int checkedMultiply(int left, int right) {
    constexpr int min{std::numeric_limits<int>::min()};
    constexpr int max{std::numeric_limits<int>::max()};

    if (left == 0 || right == 0) {
        return 0;
    }

    if (left > 0) {
        if ((right > 0 && left > max / right) ||
            (right < 0 && right < min / left)) {
            throw std::runtime_error("integer overflow in multiplication");
        }
    } else {
        if ((right > 0 && left < min / right) ||
            (right < 0 && left < max / right)) {
            throw std::runtime_error("integer overflow in multiplication");
        }
    }

    return left * right;
}

/**
 * @brief Divides two integers while rejecting zero and INT_MIN / -1.
 */
inline int checkedDivide(int left, int right) {
    constexpr int min{std::numeric_limits<int>::min()};

    if (right == 0) {
        throw std::runtime_error("integer division by zero");
    }

    if (left == min && right == -1) {
        throw std::runtime_error("integer overflow in division");
    }

    return left / right;
}

/**
 * @brief Computes integer remainder without invoking C++ division overflow UB.
 *
 * INT_MIN % -1 is mathematically zero. NovaC defines that result explicitly
 * instead of evaluating the undefined C++ expression.
 */
inline int checkedModulo(int left, int right) {
    constexpr int min{std::numeric_limits<int>::min()};

    if (right == 0) {
        throw std::runtime_error("integer modulo by zero");
    }

    if (left == min && right == -1) {
        return 0;
    }

    return left % right;
}

/** @brief Negates an integer and rejects negation of INT_MIN. */
inline int checkedNegate(int value) {
    constexpr int min{std::numeric_limits<int>::min()};

    if (value == min) {
        throw std::runtime_error("integer overflow in negation");
    }

    return -value;
}

} // namespace novac::assets::atomic::operations::detail
