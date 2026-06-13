#pragma once

#include "novac/assets/atomic/LiteralFeature.hpp"

#include <string>

namespace novac::assets::atomic::literals {

struct BooleanLiteralTokens {
    std::string trueToken{"true"};
    std::string falseToken{"false"};
};

class BooleanLiteralAtomic final : public LiteralFeature {
public:
    explicit BooleanLiteralAtomic(
        std::string nodeKind = "BooleanLiteral",
        BooleanLiteralTokens tokens = {});

    LiteralInfo info() const override;
    void install(AtomicController &controller) const override;

private:
    std::string nodeKind_;
    BooleanLiteralTokens tokens_;
};

} // namespace novac::assets::atomic::literals
