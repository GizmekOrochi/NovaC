#pragma once

#include "novac/assets/atomic/OperationFeature.hpp"

#include <string>

namespace novac::assets::atomic::operations {

class NumericComparisonOperationAtomic : public OperationFeature {
public:
    NumericComparisonOperationAtomic(
        std::string id,
        std::string description,
        TokenPattern pattern);

    OperationInfo info() const override;
    void install(AtomicController &controller) const override;

protected:
    virtual bool compare(double left, double right) const = 0;

private:
    std::string id_;
    std::string description_;
    TokenPattern pattern_;
};

class EqualOperationAtomic final : public NumericComparisonOperationAtomic {
public:
    explicit EqualOperationAtomic(TokenPattern pattern = TokenPattern::text("=="));
private:
    bool compare(double left, double right) const override;
};

class NotEqualOperationAtomic final : public NumericComparisonOperationAtomic {
public:
    explicit NotEqualOperationAtomic(TokenPattern pattern = TokenPattern::text("!="));
private:
    bool compare(double left, double right) const override;
};

class LessOperationAtomic final : public NumericComparisonOperationAtomic {
public:
    explicit LessOperationAtomic(TokenPattern pattern = TokenPattern::text("<"));
private:
    bool compare(double left, double right) const override;
};

class LessEqualOperationAtomic final : public NumericComparisonOperationAtomic {
public:
    explicit LessEqualOperationAtomic(TokenPattern pattern = TokenPattern::text("<="));
private:
    bool compare(double left, double right) const override;
};

class GreaterOperationAtomic final : public NumericComparisonOperationAtomic {
public:
    explicit GreaterOperationAtomic(TokenPattern pattern = TokenPattern::text(">"));
private:
    bool compare(double left, double right) const override;
};

class GreaterEqualOperationAtomic final : public NumericComparisonOperationAtomic {
public:
    explicit GreaterEqualOperationAtomic(TokenPattern pattern = TokenPattern::text(">="));
private:
    bool compare(double left, double right) const override;
};

} // namespace novac::assets::atomic::operations
