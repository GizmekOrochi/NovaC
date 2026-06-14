#pragma once

#include "novac/assets/atomic/LiteralFeature.hpp"
#include "novac/assets/atomic/OperationFeature.hpp"
#include "novac/engine/EngineController.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>

namespace novac::assets::atomic {

struct AtomicControllerOptions {
    std::string expressionDomain{"expr"};
    std::string binaryNodeKind{"BinaryExpr"};
    std::string unaryNodeKind{"UnaryExpr"};
};

class AtomicController {
public:
    explicit AtomicController(controllers::EngineController &engine, AtomicControllerOptions options = {});

    AtomicController &use(const LiteralFeature &feature);
    AtomicController &use(const OperationFeature &feature);

    AtomicController &integer(std::string nodeKind = "IntegerLiteral");
    AtomicController &integer(std::string nodeKind, std::vector<std::string> suffixes);
    AtomicController &integerSuffix(std::string suffix, std::string nodeKind = "IntegerLiteral");
    AtomicController &integerSuffixRegex(std::string suffixRegex, std::string nodeKind = "IntegerLiteral");

    AtomicController &floating(std::string nodeKind = "FloatLiteral");
    AtomicController &floating(std::string nodeKind, std::vector<std::string> suffixes);
    AtomicController &floatingSuffix(std::string suffix, std::string nodeKind = "FloatLiteral");
    AtomicController &floatingSuffixRegex(std::string suffixRegex, std::string nodeKind = "FloatLiteral");

    AtomicController &stringLiteral(std::string nodeKind = "StringLiteral");
    AtomicController &stringLiteral(std::string nodeKind, std::vector<std::string> suffixes);
    AtomicController &stringSuffix(std::string suffix, std::string nodeKind = "StringLiteral");
    AtomicController &stringSuffixRegex(std::string suffixRegex, std::string nodeKind = "StringLiteral");

    AtomicController &boolean(std::string nodeKind = "BooleanLiteral", std::string trueToken = "true", std::string falseToken = "false");

    AtomicController &add(std::string token = "+");
    AtomicController &subtract(std::string token = "-");
    AtomicController &multiply(std::string token = "*");
    AtomicController &divide(std::string token = "/");
    AtomicController &modulo(std::string token = "%");

    AtomicController &equal(std::string token = "==");
    AtomicController &notEqual(std::string token = "!=");
    AtomicController &less(std::string token = "<");
    AtomicController &lessEqual(std::string token = "<=");
    AtomicController &greater(std::string token = ">");
    AtomicController &greaterEqual(std::string token = ">=");

    AtomicController &logicalAnd(std::string token = "&&");
    AtomicController &logicalOr(std::string token = "||");
    AtomicController &logicalNot(std::string token = "!");
    AtomicController &negate(std::string token = "-");

    AtomicController &own(std::unique_ptr<LiteralFeature> feature);
    AtomicController &own(std::unique_ptr<OperationFeature> feature);

    AtomicController &standardLiterals();
    AtomicController &standardNumericOperations();
    AtomicController &standardComparisonOperations();
    AtomicController &standardLogicalOperations();
    AtomicController &standardOperations();
    AtomicController &standardCore();

    controllers::EngineController &engine();
    const controllers::EngineController &engine() const;

    const std::string &expressionDomain() const;
    const std::string &binaryNodeKind() const;
    const std::string &unaryNodeKind() const;

    bool hasLiteral(const std::string &id) const;
    bool hasOperation(const std::string &id) const;

    const std::vector<LiteralInfo> &literals() const;
    const std::vector<OperationInfo> &operations() const;

    void registerPattern(const TokenPattern &pattern);
    void ensureBinaryExpressionNode();
    void ensureUnaryExpressionNode();
    void registerUnaryOperation(std::string operationId, runtime::ExprHandler handler);

private:
    static TokenPattern tokenPattern(std::string token);
    static TokenPattern suffixPattern(std::string tokenKey, std::string suffix);
    static TokenPattern suffixRegexPattern(std::string tokenKey, std::string suffixRegex);
    static std::string suffixRegexFromList(const std::vector<std::string> &suffixes);

    void validateLiteral(const LiteralInfo &info) const;
    void validateOperation(const OperationInfo &info) const;
    void rememberLiteral(LiteralInfo info);
    void rememberOperation(OperationInfo info);

    controllers::EngineController &engine_;
    AtomicControllerOptions options_;
    std::vector<LiteralInfo> literals_;
    std::vector<OperationInfo> operations_;
    std::vector<std::unique_ptr<LiteralFeature>> ownedLiterals_;
    std::vector<std::unique_ptr<OperationFeature>> ownedOperations_;
    std::unordered_set<std::string> literalIds_;
    std::unordered_set<std::string> operationIds_;
    std::unordered_set<std::string> registeredPatterns_;
    std::unordered_map<std::string, runtime::ExprHandler> unaryHandlers_;
    bool binaryNodeInstalled_{};
    bool unaryNodeInstalled_{};
};

} // namespace novac::assets::atomic