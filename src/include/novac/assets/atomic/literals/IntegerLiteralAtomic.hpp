#pragma once

#include "novac/assets/atomic/LiteralFeature.hpp"

#include <string>

namespace novac::assets::atomic::literals {

class IntegerLiteralAtomic final : public LiteralFeature {
public:
    explicit IntegerLiteralAtomic(
        std::string nodeKind = "IntegerLiteral",
        TokenPattern pattern = TokenPattern::key("$int"));

    LiteralInfo info() const override;
    void install(AtomicController &controller) const override;

private:
    std::string nodeKind_;
    TokenPattern pattern_;
};

} // namespace novac::assets::atomic::literals
