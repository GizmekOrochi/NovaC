#pragma once

#include "novac/assets/language/LanguageFeature.hpp"

namespace novac::language::features {

class ExpressionStatementFeature final : public LanguageFeature {
public:
    explicit ExpressionStatementFeature(std::string statementDomain = "stmt", std::string expressionDomain = "expr", std::string nodeKind = "ExpressionStatement");

    LanguageFeatureInfo info() const override;
    void install(LanguageOptionsController &language) const override;

private:
    std::string statementDomain_;
    std::string expressionDomain_;
    std::string nodeKind_;
};

} // namespace novac::language::features
