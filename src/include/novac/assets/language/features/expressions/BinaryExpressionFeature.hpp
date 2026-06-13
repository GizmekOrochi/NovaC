#pragma once

#include "novac/assets/language/LanguageFeature.hpp"

namespace novac::language::features {

class BinaryExpressionFeature final : public LanguageFeature {
public:
    explicit BinaryExpressionFeature(std::string expressionDomain = "expr", std::string nodeKind = "BinaryExpr");

    LanguageFeatureInfo info() const override;
    void install(LanguageOptionsController &language) const override;

private:
    std::string expressionDomain_;
    std::string nodeKind_;
};

} // namespace novac::language::features
