#include "novac/assets/compilation/target/TargetRegistry.hpp"
#include <stdexcept>
#include <algorithm>
#include <tuple>
#include <utility>

namespace novac::compilation::target {
void TargetRegistry::add(std::string id, std::shared_ptr<const backend::NativeTarget> provider) {
    if (frozen_) throw std::runtime_error("TargetRegistry: registry is frozen");
    if (id.empty() || !provider) throw std::runtime_error("TargetRegistry: target id and provider are required");
    provider->specification().validate();
    if (targets_.contains(id)) throw std::runtime_error("TargetRegistry: duplicate id '" + id + "'");
    targets_.emplace(std::move(id),std::move(provider));
}
bool TargetRegistry::has(const std::string &id) const noexcept { return targets_.contains(id); }
const backend::NativeTarget& TargetRegistry::require(const std::string &id) const {
    const auto it=targets_.find(id);
    if (it==targets_.end()) throw std::runtime_error("TargetRegistry: unknown target id '" + id + "'");
    return *it->second;
}
std::shared_ptr<const backend::NativeTarget> TargetRegistry::requireShared(const std::string &id) const {
    const auto it = targets_.find(id);
    if (it == targets_.end()) throw std::runtime_error("TargetRegistry: unknown target id '" + id + "'");
    return it->second;
}
std::vector<std::string> TargetRegistry::providersFor(const TargetSpec &requested) const {
    requested.validate();
    std::vector<std::string> ids;
    for (const auto &[id, provider] : targets_)
        if (provider->specification() == requested) ids.push_back(id);
    std::sort(ids.begin(), ids.end());
    return ids;
}
std::shared_ptr<const backend::NativeTarget> TargetRegistry::resolveShared(
        const TargetSpec &requested, const std::string &providerId) const {
    requested.validate();
    auto selected = requireShared(providerId);
    if (selected->specification() != requested)
        throw std::runtime_error("TargetRegistry: provider '" + providerId +
                                 "' does not support requested target '" + requested.display() + "'");
    return selected;
}
std::shared_ptr<const backend::NativeTarget> TargetRegistry::resolveShared(const TargetSpec& requested) const {
    auto ids = providersFor(requested);
    if (ids.empty())
        throw std::runtime_error("TargetRegistry: unsupported target '" + requested.display() + "'");
    if (ids.size() != 1) {
        std::string detail;
        for (const auto &id : ids) detail += (detail.empty() ? "" : ", ") + id;
        throw std::runtime_error("TargetRegistry: ambiguous target '" + requested.display() +
                                 "'; select a provider ID: " + detail);
    }
    return requireShared(ids.front());
}
const backend::NativeTarget& TargetRegistry::resolve(const TargetSpec& requested) const {
    return *resolveShared(requested);
}
std::vector<TargetSpec> TargetRegistry::available() const {
    std::vector<TargetSpec> specs;
    specs.reserve(targets_.size());
    for (const auto &[id, item] : targets_) specs.push_back(item->specification());
    std::sort(specs.begin(), specs.end(), [](const TargetSpec &a, const TargetSpec &b) {
        return std::tie(a.architecture,a.operatingSystem,a.abi,a.objectFormat,a.cpu,a.cpuFeatures) <
               std::tie(b.architecture,b.operatingSystem,b.abi,b.objectFormat,b.cpu,b.cpuFeatures);
    });
    return specs;
}
} // namespace novac::compilation::target
