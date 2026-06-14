#pragma once

namespace novac::registry {

/**
 * @brief Policy applied when a registry receives a duplicate key.
 */
enum class DuplicatePolicy {
    /**
     * @brief Reject duplicate registrations.
     */
    Error,

    /**
     * @brief Replace the existing entry with the new one.
     */
    Replace,

    /**
     * @brief Keep the existing entry and ignore the new one.
     */
    Ignore
};

/**
 * @brief Result returned by registry insertion operations.
 */
enum class RegisterStatus {
    /**
     * @brief A new entry was inserted.
     */
    Inserted,

    /**
     * @brief An existing entry was replaced.
     */
    Replaced,

    /**
     * @brief A duplicate entry was ignored.
     */
    Ignored
};

} // namespace novac::registry