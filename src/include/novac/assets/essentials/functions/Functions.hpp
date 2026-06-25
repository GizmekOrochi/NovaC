#pragma once

#include "novac/assets/essentials/EssentialFeature.hpp"

namespace novac::assets::essentials::functions {

class FunctionsFeature final : public EssentialFeature {
public:
    EssentialInfo info() const override;
    void install(EssentialsController &controller) const override;
};

EssentialPack functions();
EssentialPack standard();

} // namespace novac::assets::essentials::functions
