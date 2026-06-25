#pragma once

#include "novac/assets/essentials/EssentialFeature.hpp"

namespace novac::assets::essentials::scopes {

class ScopedBlocksFeature final : public EssentialFeature {
public:
    EssentialInfo info() const override;
    void install(EssentialsController &controller) const override;
};

EssentialPack scopedBlocks();
EssentialPack standard();

} // namespace novac::assets::essentials::scopes
