#include "novac/assets/essentials/EssentialFeature.hpp"

#include <utility>

namespace novac::assets::essentials {

EssentialFeature::~EssentialFeature() = default;

EssentialPack &EssentialPack::merge(EssentialPack pack) {
    for (auto &feature : pack.features) {
        features.push_back(std::move(feature));
    }

    pack.features.clear();
    return *this;
}

} // namespace novac::assets::essentials
