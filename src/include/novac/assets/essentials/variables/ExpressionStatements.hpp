#pragma once

#include "novac/assets/essentials/EssentialFeature.hpp"

namespace novac::assets::essentials::variables {

class ExpressionStatementsFeature final : public EssentialFeature {
public:
    EssentialInfo info() const override;
    void install(EssentialsController &controller) const override;
};

EssentialPack expressionStatements();
EssentialPack standard();

} // namespace novac::assets::essentials::variables
