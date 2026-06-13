#pragma once

#include "novac/assets/atomic/LiteralFeature.hpp"

#include <string>

namespace novac::assets::atomic::literals {

class FloatLiteralAtomic final : public LiteralFeature {
public:
    explicit FloatLiteralAtomic(
        std::string nodeKind = "FloatLiteral",
        TokenPattern pattern = TokenPattern::key("$float"));

    LiteralInfo info() const override;
    void install(AtomicController &controller) const override;

private:
    std::string nodeKind_;
    TokenPattern pattern_;
};

} // namespace novac::assets::atomic::literals
