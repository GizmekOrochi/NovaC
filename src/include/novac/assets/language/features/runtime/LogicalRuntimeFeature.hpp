#pragma once

#include "novac/assets/language/LanguageFeature.hpp"

namespace novac::language::features {

class LogicalRuntimeFeature final : public LanguageFeature {
public:
    explicit LogicalRuntimeFeature(std::string binaryNodeKind = "BinaryExpr");

    LanguageFeatureInfo info() const override;
    void install(LanguageOptionsController &language) const override;

private:
    std::string binaryNodeKind_;
};

} // namespace novac::language::features
