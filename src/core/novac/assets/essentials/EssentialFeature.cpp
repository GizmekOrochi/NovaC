#include "novac/assets/essentials/EssentialFeature.hpp"

#include <utility>

namespace novac::assets::essentials {

/**
 * @brief Destroys the `EssentialFeature` instance.
 */
EssentialFeature::~EssentialFeature() = default;

/**
 * @brief Adds the supplied behavior through `merge`.
 *
 * @param pack Value supplied for `pack`.
 * @return Value produced by the operation.
 */
EssentialPack &EssentialPack::merge(EssentialPack pack) {
    for (auto &feature : pack.features) {
        features.push_back(std::move(feature));
    }

    pack.features.clear();
    return *this;
}

} // namespace novac::assets::essentials
