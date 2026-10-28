#pragma once
#include "novac/assets/compilation/backend/NativeBackend.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace novac::compilation::target {
/** Explicit registry for native compilation destinations. Selecting a target
 * is separate from registering a backend with the general BackendRegistry. */
class TargetRegistry final {
public:
    void add(std::string id, std::shared_ptr<const backend::NativeTarget> target);
    bool has(const std::string &id) const noexcept;
    std::shared_ptr<const backend::NativeTarget> requireShared(const std::string &id) const;
    /** Explicitly resolve one provider for a specification; IDs must match the requested target. */
    std::shared_ptr<const backend::NativeTarget> resolveShared(const TargetSpec &requested,
                                                                const std::string &providerId) const;
    /** Stable sorted list of providers exactly matching a target specification. */
    std::vector<std::string> providersFor(const TargetSpec &requested) const;
    const backend::NativeTarget& require(const std::string &id) const;
    /** Ambiguous exact matches require a provider ID; never silently pick one.
     * Requires an exact architecture, OS, ABI, format, CPU and feature set.
     * There is no silent fallback to an ABI or format from another platform. */
    const backend::NativeTarget& resolve(const TargetSpec& requested) const;
    std::shared_ptr<const backend::NativeTarget> resolveShared(const TargetSpec& requested) const;
    std::vector<TargetSpec> available() const;
    void freeze() noexcept { frozen_ = true; }
    bool frozen() const noexcept { return frozen_; }
private:
    std::unordered_map<std::string,std::shared_ptr<const backend::NativeTarget>> targets_{};
    bool frozen_{false};
};
} // namespace novac::compilation::target
