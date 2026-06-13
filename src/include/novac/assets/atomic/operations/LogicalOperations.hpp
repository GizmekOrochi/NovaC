#pragma once

#include "novac/assets/atomic/OperationFeature.hpp"

#include <string>

namespace novac::assets::atomic::operations {

class LogicalAndOperationAtomic final : public OperationFeature {
public:
    explicit LogicalAndOperationAtomic(TokenPattern pattern = TokenPattern::text("&&"));

    OperationInfo info() const override;
    void install(AtomicController &controller) const override;

private:
    TokenPattern pattern_;
};

class LogicalOrOperationAtomic final : public OperationFeature {
public:
    explicit LogicalOrOperationAtomic(TokenPattern pattern = TokenPattern::text("||"));

    OperationInfo info() const override;
    void install(AtomicController &controller) const override;

private:
    TokenPattern pattern_;
};

class LogicalNotOperationAtomic final : public OperationFeature {
public:
    explicit LogicalNotOperationAtomic(TokenPattern pattern = TokenPattern::text("!"));

    OperationInfo info() const override;
    void install(AtomicController &controller) const override;

private:
    TokenPattern pattern_;
};


class NumericNegateOperationAtomic final : public OperationFeature {
public:
    explicit NumericNegateOperationAtomic(TokenPattern pattern = TokenPattern::text("-"));

    OperationInfo info() const override;
    void install(AtomicController &controller) const override;

private:
    TokenPattern pattern_;
};

} // namespace novac::assets::atomic::operations
