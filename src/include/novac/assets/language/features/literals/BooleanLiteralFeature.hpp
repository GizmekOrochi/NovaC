#pragma once

#include "novac/assets/language/LanguageFeature.hpp"

namespace novac::language::features {

class BooleanLiteralFeature final : public LanguageFeature {
public:
    explicit BooleanLiteralFeature(std::string expressionDomain = "expr", std::string nodeKind = "BoolLiteral");

    LanguageFeatureInfo info() const override;
    void install(LanguageOptionsController &language) const override;

private:
    std::string expressionDomain_;
    std::string nodeKind_;
};

} // namespace novac::language::features
