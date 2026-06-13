#pragma once

#include "../LanguageFeature.hpp"

#include <string>

namespace novac::language::features {

class BooleanLiteralFeature final : public LanguageFeature {
public:
    explicit BooleanLiteralFeature(std::string expressionDomain = "expr", std::string nodeKind = "BooleanLiteral");

    LanguageFeatureInfo info() const override;
    void install(LanguageOptionsController &language) const override;

private:
    std::string expressionDomain_;
    std::string nodeKind_;
};

} // namespace novac::language::features
