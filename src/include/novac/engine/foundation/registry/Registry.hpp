#pragma once

namespace novac::registry {

/**
 * @brief Policy applied when a registry receives a duplicate key.
 *
 * This policy controls what happens when a new entry is registered using a
 * key that already exists in the registry.
 */
enum class DuplicatePolicy {
    /**
     * @brief Reject duplicate registrations.
     *
     * The registry should report an error instead of modifying the existing
     * entry.
     */
    Error,

    /**
     * @brief Replace the existing entry with the new one.
     *
     * The previous value associated with the key is overwritten.
     */
    Replace,

    /**
     * @brief Keep the existing entry and ignore the new one.
     *
     * The registry remains unchanged when the duplicate is encountered.
     */
    Ignore
};

/**
 * @brief Result returned by registry insertion operations.
 *
 * RegisterStatus describes what actually happened during registration, while
 * DuplicatePolicy describes what behavior should be used when a duplicate key
 * is encountered.
 */
enum class RegisterStatus {
    /**
     * @brief A new entry was inserted.
     *
     * The key did not previously exist in the registry.
     */
    Inserted,

    /**
     * @brief An existing entry was replaced.
     *
     * This normally occurs when the registry uses DuplicatePolicy::Replace.
     */
    Replaced,

    /**
     * @brief A duplicate entry was ignored.
     *
     * The existing entry remains unchanged.
     */
    Ignored
};

} // namespace novac::registry