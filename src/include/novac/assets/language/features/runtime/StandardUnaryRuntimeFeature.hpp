#pragma once

#include "novac/assets/language/LanguageFeature.hpp"

namespace novac::language::features {

class StandardUnaryRuntimeFeature final : public LanguageFeature {
public:
    explicit StandardUnaryRuntimeFeature(std::string unaryNodeKind = "UnaryExpr");

    LanguageFeatureInfo info() const override;
    void install(LanguageOptionsController &language) const override;

private:
    std::string unaryNodeKind_;
};

} // namespace novac::language::features
