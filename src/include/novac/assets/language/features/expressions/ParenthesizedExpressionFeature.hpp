#pragma once

#include "novac/assets/language/LanguageFeature.hpp"

namespace novac::language::features {

class ParenthesizedExpressionFeature final : public LanguageFeature {
public:
    explicit ParenthesizedExpressionFeature(std::string expressionDomain = "expr");

    LanguageFeatureInfo info() const override;
    void install(LanguageOptionsController &language) const override;

private:
    std::string expressionDomain_;
};

} // namespace novac::language::features
