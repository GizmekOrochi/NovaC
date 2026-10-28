#pragma once
#include <set>
#include <string>

namespace novac::compilation::target {
/** A compilation destination is a tuple, not just a CPU architecture.
 * This identifies the supported OS, calling convention, and binary format;
 * new architectures register providers without changing the controller. */
struct TargetSpec {
    std::string architecture;
    std::string operatingSystem;
    std::string abi;
    std::string objectFormat;
    std::string cpu{"generic"};
    std::set<std::string> cpuFeatures{};

    void validate() const;
    bool operator==(const TargetSpec&) const = default;
    std::string display() const;
};
TargetSpec linuxX86_64();
} // namespace novac::compilation::target
