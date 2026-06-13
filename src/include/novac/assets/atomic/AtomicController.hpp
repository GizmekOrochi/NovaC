#pragma once

#include "novac/assets/atomic/LiteralFeature.hpp"
#include "novac/assets/atomic/OperationFeature.hpp"
#include "novac/engine/EngineController.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace novac::assets::atomic {

struct AtomicControllerOptions {
    std::string expressionDomain{"expr"};
    std::string binaryNodeKind{"BinaryExpr"};
    std::string unaryNodeKind{"UnaryExpr"};
};

class AtomicController {
public:
    explicit AtomicController(
        controllers::EngineController &engine,
        AtomicControllerOptions options = {});

    AtomicController &use(const LiteralFeature &feature);
    AtomicController &use(const OperationFeature &feature);

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
    void validateLiteral(const LiteralInfo &info) const;
    void validateOperation(const OperationInfo &info) const;
    void rememberLiteral(LiteralInfo info);
    void rememberOperation(OperationInfo info);

    controllers::EngineController &engine_;
    AtomicControllerOptions options_;
    std::vector<LiteralInfo> literals_;
    std::vector<OperationInfo> operations_;
    std::unordered_set<std::string> literalIds_;
    std::unordered_set<std::string> operationIds_;
    std::unordered_set<std::string> registeredPatterns_;
    std::unordered_map<std::string, runtime::ExprHandler> unaryHandlers_;
    bool binaryNodeInstalled_{};
    bool unaryNodeInstalled_{};
};

} // namespace novac::assets::atomic
