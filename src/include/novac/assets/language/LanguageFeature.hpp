#pragma once

#include <string>
#include <vector>

namespace novac::language {

class LanguageOptionsController;

struct VersionRequirement final {
    std::string feature{};
    std::string minVersion{};
};

struct LanguageFeatureInfo final {
    std::string name{};
    std::string version{"0.1.0"};
    std::string description{};
    std::vector<std::string> capabilities{};
    std::vector<std::string> requiredCapabilities{};
    std::vector<std::string> dependencies{};
    std::vector<std::string> conflicts{};
    std::vector<VersionRequirement> versionRequirements{};
};

class LanguageFeature {
public:
    virtual ~LanguageFeature();

    virtual LanguageFeatureInfo info() const = 0;
    virtual void install(LanguageOptionsController &language) const = 0;
};

} // namespace novac::language
