#pragma once

#include "../LanguageFeature.hpp"

#include <string>

namespace novac::language::features {

class IdentifierExpressionFeature final : public LanguageFeature {
public:
    explicit IdentifierExpressionFeature(std::string expressionDomain = "expr", std::string nodeKind = "IdentifierExpr");

    LanguageFeatureInfo info() const override;
    void install(LanguageOptionsController &language) const override;

private:
    std::string expressionDomain_;
    std::string nodeKind_;
};

} // namespace novac::language::features
