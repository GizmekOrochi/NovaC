#include "novac/assets/atomic/AtomicController.hpp"

#include <stdexcept>
#include <utility>

namespace novac::assets::atomic {

AtomicController::AtomicController(controllers::EngineController &engine, AtomicControllerOptions options)
    : engine_{engine}, options_{std::move(options)}, literals_{}, operations_{},
      literalIds_{}, operationIds_{}, registeredPatterns_{},
      unaryHandlers_{}, binaryNodeInstalled_{}, unaryNodeInstalled_{} {
    if (options_.expressionDomain.empty()) {
        throw std::runtime_error("AtomicController::AtomicController: expression domain cannot be empty");
    }

    if (options_.binaryNodeKind.empty()) {
        throw std::runtime_error("AtomicController::AtomicController: binary node kind cannot be empty");
    }

    if (options_.unaryNodeKind.empty()) {
        throw std::runtime_error("AtomicController::AtomicController: unary node kind cannot be empty");
    }

    engine_.setStartDomain(options_.expressionDomain);
}

AtomicController &AtomicController::use(const LiteralFeature &feature) {
    LiteralInfo info{feature.info()};
    validateLiteral(info);
    feature.install(*this);
    rememberLiteral(std::move(info));
    return *this;
}

AtomicController &AtomicController::use(const OperationFeature &feature) {
    OperationInfo info{feature.info()};
    validateOperation(info);
    feature.install(*this);
    rememberOperation(std::move(info));
    return *this;
}

controllers::EngineController &AtomicController::engine() {
    return engine_;
}

const controllers::EngineController &AtomicController::engine() const {
    return engine_;
}

const std::string &AtomicController::expressionDomain() const {
    return options_.expressionDomain;
}

const std::string &AtomicController::binaryNodeKind() const {
    return options_.binaryNodeKind;
}

const std::string &AtomicController::unaryNodeKind() const {
    return options_.unaryNodeKind;
}

bool AtomicController::hasLiteral(const std::string &id) const {
    return literalIds_.find(id) != literalIds_.end();
}

bool AtomicController::hasOperation(const std::string &id) const {
    return operationIds_.find(id) != operationIds_.end();
}

const std::vector<LiteralInfo> &AtomicController::literals() const {
    return literals_;
}

const std::vector<OperationInfo> &AtomicController::operations() const {
    return operations_;
}

void AtomicController::registerPattern(const TokenPattern &pattern) {
    if (pattern.token.empty()) {
        return;
    }

    const std::string key{pattern.keyword ? "keyword:" + pattern.token : "symbol:" + pattern.token};

    if (registeredPatterns_.find(key) != registeredPatterns_.end()) {
        return;
    }

    if (pattern.keyword) {
        engine_.keyword(pattern.token);
    } else {
        engine_.symbol(pattern.token);
    }

    registeredPatterns_.insert(key);
}

void AtomicController::ensureBinaryExpressionNode() {
    if (binaryNodeInstalled_) {
        return;
    }

    engine_.node({
        .kind = options_.binaryNodeKind,
        .fields = {
            {.name = "op", .kind = ast::FieldKind::String, .required = true},
            {.name = "left", .kind = ast::FieldKind::Node, .required = true, .allowedNodeTraits = {"expr"}},
            {.name = "right", .kind = ast::FieldKind::Node, .required = true, .allowedNodeTraits = {"expr"}}
        },
        .traits = {"expr"},
        .doc = "Atomic binary expression carrier"
    });

    engine_.setBinaryNodeKind(options_.binaryNodeKind);
    binaryNodeInstalled_ = true;
}

void AtomicController::ensureUnaryExpressionNode() {
    if (unaryNodeInstalled_) {
        return;
    }

    engine_.node({
        .kind = options_.unaryNodeKind,
        .fields = {
            {.name = "op", .kind = ast::FieldKind::String, .required = true},
            {.name = "expr", .kind = ast::FieldKind::Node, .required = true, .allowedNodeTraits = {"expr"}}
        },
        .traits = {"expr"},
        .doc = "Atomic unary expression carrier"
    });

    engine_.expression(options_.unaryNodeKind, [this](const ast::Node &node, runtime::RuntimeContext &context) {
        const std::string op{node.str("op")};
        const auto iter{unaryHandlers_.find(op)};

        if (iter == unaryHandlers_.end()) {
            throw std::runtime_error("AtomicController::ensureUnaryExpressionNode: missing unary operation handler for '" + op + "'");
        }

        return iter->second(node, context);
    });

    unaryNodeInstalled_ = true;
}

void AtomicController::registerUnaryOperation(std::string operationId, runtime::ExprHandler handler) {
    if (operationId.empty()) {
        throw std::runtime_error("AtomicController::registerUnaryOperation: operation id cannot be empty");
    }

    if (!handler) {
        throw std::runtime_error("AtomicController::registerUnaryOperation: handler cannot be empty");
    }

    const auto result{unaryHandlers_.emplace(std::move(operationId), std::move(handler))};

    if (!result.second) {
        throw std::runtime_error("AtomicController::registerUnaryOperation: duplicate unary operation handler");
    }
}

void AtomicController::validateLiteral(const LiteralInfo &info) const {
    if (info.id.empty()) {
        throw std::runtime_error("AtomicController::validateLiteral: literal id cannot be empty");
    }

    if (info.nodeKind.empty()) {
        throw std::runtime_error("AtomicController::validateLiteral: literal node kind cannot be empty");
    }

    if (hasLiteral(info.id)) {
        throw std::runtime_error("AtomicController::validateLiteral: duplicate literal '" + info.id + "'");
    }
}

void AtomicController::validateOperation(const OperationInfo &info) const {
    if (info.id.empty()) {
        throw std::runtime_error("AtomicController::validateOperation: operation id cannot be empty");
    }

    if (info.pattern.token.empty() && info.pattern.tokenKey.empty()) {
        throw std::runtime_error("AtomicController::validateOperation: operation pattern cannot be empty");
    }

    if (hasOperation(info.id)) {
        throw std::runtime_error("AtomicController::validateOperation: duplicate operation '" + info.id + "'");
    }
}

void AtomicController::rememberLiteral(LiteralInfo info) {
    literalIds_.insert(info.id);
    literals_.push_back(std::move(info));
}

void AtomicController::rememberOperation(OperationInfo info) {
    operationIds_.insert(info.id);
    operations_.push_back(std::move(info));
}

} // namespace novac::assets::atomic
