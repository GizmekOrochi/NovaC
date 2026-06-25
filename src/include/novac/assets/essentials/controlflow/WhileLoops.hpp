#pragma once

#include "novac/assets/essentials/EssentialFeature.hpp"

namespace novac::assets::essentials::controlflow {

class WhileLoopsFeature final : public EssentialFeature {
public:
    EssentialInfo info() const override;
    void install(EssentialsController &controller) const override;
};

EssentialPack whileLoops();
EssentialPack standard();

} // namespace novac::assets::essentials::controlflow
