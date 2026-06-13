#pragma once

#include "../LanguageFeature.hpp"

#include <string>

namespace novac::language::features {

class UnaryExpressionFeature final : public LanguageFeature {
public:
    explicit UnaryExpressionFeature(std::string expressionDomain = "expr", std::string nodeKind = "UnaryExpr");

    LanguageFeatureInfo info() const override;
    void install(LanguageOptionsController &language) const override;

private:
    std::string expressionDomain_;
    std::string nodeKind_;
};

} // namespace novac::language::features
