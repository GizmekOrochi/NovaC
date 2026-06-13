#pragma once

#include "novac/assets/atomic/OperationFeature.hpp"

#include <string>

namespace novac::assets::atomic::operations {

class NumericBinaryOperationAtomic : public OperationFeature {
public:
    NumericBinaryOperationAtomic(
        std::string id,
        std::string description,
        TokenPattern pattern,
        int precedence);

    OperationInfo info() const override;
    void install(AtomicController &controller) const override;

protected:
    virtual runtime::Value evaluate(
        const runtime::Value &left,
        const runtime::Value &right) const = 0;

private:
    std::string id_;
    std::string description_;
    TokenPattern pattern_;
    int precedence_;
};

class AddOperationAtomic final : public NumericBinaryOperationAtomic {
public:
    explicit AddOperationAtomic(TokenPattern pattern = TokenPattern::text("+"));

private:
    runtime::Value evaluate(const runtime::Value &left, const runtime::Value &right) const override;
};

class SubtractOperationAtomic final : public NumericBinaryOperationAtomic {
public:
    explicit SubtractOperationAtomic(TokenPattern pattern = TokenPattern::text("-"));

private:
    runtime::Value evaluate(const runtime::Value &left, const runtime::Value &right) const override;
};

class MultiplyOperationAtomic final : public NumericBinaryOperationAtomic {
public:
    explicit MultiplyOperationAtomic(TokenPattern pattern = TokenPattern::text("*"));

private:
    runtime::Value evaluate(const runtime::Value &left, const runtime::Value &right) const override;
};

class DivideOperationAtomic final : public NumericBinaryOperationAtomic {
public:
    explicit DivideOperationAtomic(TokenPattern pattern = TokenPattern::text("/"));

private:
    runtime::Value evaluate(const runtime::Value &left, const runtime::Value &right) const override;
};

class ModuloOperationAtomic final : public NumericBinaryOperationAtomic {
public:
    explicit ModuloOperationAtomic(TokenPattern pattern = TokenPattern::text("%"));

private:
    runtime::Value evaluate(const runtime::Value &left, const runtime::Value &right) const override;
};

} // namespace novac::assets::atomic::operations
