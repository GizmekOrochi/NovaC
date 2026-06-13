#pragma once

#include "novac/assets/language/LanguageFeature.hpp"

namespace novac::language::features {

class UnaryOperatorSyntaxFeature final : public LanguageFeature {
public:
    explicit UnaryOperatorSyntaxFeature(std::string expressionDomain = "expr", std::string unaryNodeKind = "UnaryExpr");

    LanguageFeatureInfo info() const override;
    void install(LanguageOptionsController &language) const override;

private:
    std::string expressionDomain_;
    std::string unaryNodeKind_;
};

} // namespace novac::language::features
