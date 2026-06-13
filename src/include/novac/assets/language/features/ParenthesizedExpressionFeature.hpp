#pragma once

#include "../LanguageFeature.hpp"

#include <string>

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
