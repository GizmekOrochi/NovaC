#pragma once

#include "../LanguageFeature.hpp"

#include <string>

namespace novac::language::features {

class ArithmeticOperatorsFeature final : public LanguageFeature {
public:
    explicit ArithmeticOperatorsFeature(std::string expressionDomain = "expr", std::string binaryNodeKind = "BinaryExpr");

    LanguageFeatureInfo info() const override;
    void install(LanguageOptionsController &language) const override;

private:
    std::string expressionDomain_;
    std::string binaryNodeKind_;
};

} // namespace novac::language::features
