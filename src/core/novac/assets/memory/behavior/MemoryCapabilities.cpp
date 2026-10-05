#include "novac/assets/memory/behavior/MemoryCapabilities.hpp"

namespace novac::assets::memory {

void MemoryCapabilitySet::clear() {
    ensureMutable();
    values_.clear();
}

void MemoryCapabilitySet::freeze() noexcept {
    frozen_ = true;
}

bool MemoryCapabilitySet::frozen() const noexcept {
    return frozen_;
}

std::size_t MemoryCapabilitySet::size() const noexcept {
    return values_.size();
}

bool MemoryCapabilitySet::empty() const noexcept {
    return values_.empty();
}

void MemoryCapabilitySet::ensureMutable() const {
    if (frozen_)
        throw std::runtime_error("MemoryCapabilitySet: committed behavior is immutable");
}

} // namespace novac::assets::memory
