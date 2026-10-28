#include "novac/assets/compilation/target/TargetSpec.hpp"
#include <stdexcept>
namespace novac::compilation::target {
void TargetSpec::validate() const {
    if (architecture.empty() || operatingSystem.empty() || abi.empty() || objectFormat.empty() || cpu.empty())
        throw std::runtime_error("TargetSpec: architecture, OS, ABI, object format and CPU must be specified");
    for (const auto &feature : cpuFeatures) if (feature.empty())
        throw std::runtime_error("TargetSpec: empty CPU feature");
}
std::string TargetSpec::display() const {
    std::string features;
    for (const auto &feature : cpuFeatures) features += (features.empty() ? "" : ",") + feature;
    return architecture + "-" + operatingSystem + "-" + abi + "-" + objectFormat +
           " (" + cpu + (features.empty() ? "" : "; features=" + features) + ")";
}
TargetSpec linuxX86_64() { return {"x86-64","linux","sysv-amd64","elf64","generic",{}}; }
} // namespace novac::compilation::target
