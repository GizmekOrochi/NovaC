#include "novac/assets/memory/behavior/MemoryCapabilities.hpp"

namespace novac::assets::memory {

/**
 * @brief Resets state through `clear`.
 */
void MemoryCapabilitySet::clear() {
    ensureMutable();
    values_.clear();
}

/**
 * @brief Implements the `freeze` operation.
 */
void MemoryCapabilitySet::freeze() noexcept {
    frozen_ = true;
}

/**
 * @brief Implements the `frozen` operation.
 *
 * @return Value produced by the operation.
 */
bool MemoryCapabilitySet::frozen() const noexcept {
    return frozen_;
}

/**
 * @brief Implements the `size` operation.
 *
 * @return Value produced by the operation.
 */
std::size_t MemoryCapabilitySet::size() const noexcept {
    return values_.size();
}

/**
 * @brief Checks the condition represented by `empty`.
 *
 * @return Value produced by the operation.
 */
bool MemoryCapabilitySet::empty() const noexcept {
    return values_.empty();
}

/**
 * @brief Ensures the invariant required by `ensureMutable`.
 */
void MemoryCapabilitySet::ensureMutable() const {
    if (frozen_)
        throw std::runtime_error("MemoryCapabilitySet: committed behavior is immutable");
}

} // namespace novac::assets::memory
