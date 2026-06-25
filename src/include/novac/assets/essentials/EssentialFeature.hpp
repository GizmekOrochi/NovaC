#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace novac::assets::essentials {

class EssentialsController;

struct EssentialInfo {
    std::string id{};
    std::string version{"0.1.0"};
    std::string description{};
    std::vector<std::string> nodeKinds{};
    std::vector<std::string> traits{};
    std::vector<std::string> requirements{};
};

class EssentialFeature {
public:
    virtual ~EssentialFeature();

    virtual EssentialInfo info() const = 0;
    virtual void install(EssentialsController &controller) const = 0;
};

struct EssentialPack {
    std::vector<std::unique_ptr<EssentialFeature>> features{};

    template <typename Feature, typename... Args>
    EssentialPack &add(Args &&...args) {
        features.push_back(std::make_unique<Feature>(std::forward<Args>(args)...));
        return *this;
    }

    EssentialPack &merge(EssentialPack pack);
};

} // namespace novac::assets::essentials
