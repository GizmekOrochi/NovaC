#pragma once

#include "novac/assets/language/LanguageFeature.hpp"

namespace novac::language::features {

class ComparisonOperatorSyntaxFeature final : public LanguageFeature {
public:
    explicit ComparisonOperatorSyntaxFeature(std::string expressionDomain = "expr", std::string binaryNodeKind = "BinaryExpr");

    LanguageFeatureInfo info() const override;
    void install(LanguageOptionsController &language) const override;

private:
    std::string expressionDomain_;
    std::string binaryNodeKind_;
};

} // namespace novac::language::features
