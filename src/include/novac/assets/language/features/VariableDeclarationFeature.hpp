#pragma once

#include "../LanguageFeature.hpp"

#include <string>

namespace novac::language::features {

class VariableDeclarationFeature final : public LanguageFeature {
public:
    VariableDeclarationFeature(
        std::string statementDomain = "stmt",
        std::string expressionDomain = "expr",
        std::string nodeKind = "VariableDecl");

    LanguageFeatureInfo info() const override;
    void install(LanguageOptionsController &language) const override;

private:
    std::string statementDomain_;
    std::string expressionDomain_;
    std::string nodeKind_;
};

} // namespace novac::language::features
