#pragma once

#include "novac/assets/atomic/LiteralFeature.hpp"

#include <string>

namespace novac::assets::atomic::literals {

class StringLiteralAtomic final : public LiteralFeature {
public:
    explicit StringLiteralAtomic(
        std::string nodeKind = "StringLiteral",
        TokenPattern pattern = TokenPattern::key("$string"));

    LiteralInfo info() const override;
    void install(AtomicController &controller) const override;

private:
    std::string nodeKind_;
    TokenPattern pattern_;
};

} // namespace novac::assets::atomic::literals
