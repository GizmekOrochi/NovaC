#pragma once

#include "../LanguageFeature.hpp"

#include <string>

namespace novac::language::features {

class StringLiteralFeature final : public LanguageFeature {
public:
    explicit StringLiteralFeature(std::string expressionDomain = "expr", std::string nodeKind = "StringLiteral");

    LanguageFeatureInfo info() const override;
    void install(LanguageOptionsController &language) const override;

private:
    std::string expressionDomain_;
    std::string nodeKind_;
};

} // namespace novac::language::features
