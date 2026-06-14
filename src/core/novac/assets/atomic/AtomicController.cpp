#include "novac/assets/atomic/AtomicController.hpp"

#include "novac/assets/atomic/literals/BooleanLiteralAtomic.hpp"
#include "novac/assets/atomic/literals/FloatLiteralAtomic.hpp"
#include "novac/assets/atomic/literals/IntegerLiteralAtomic.hpp"
#include "novac/assets/atomic/literals/StringLiteralAtomic.hpp"
#include "novac/assets/atomic/operations/ComparisonOperations.hpp"
#include "novac/assets/atomic/operations/LogicalOperations.hpp"
#include "novac/assets/atomic/operations/NumericOperations.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace novac::assets::atomic {

namespace {

std::string normalizeSuffix(std::string suffix) {
    if (suffix.empty()) {
        throw std::runtime_error("AtomicController::normalizeSuffix: suffix cannot be empty");
    }

    if (suffix.front() == '_') {
        suffix.erase(suffix.begin());
    }

    if (suffix.empty()) {
        throw std::runtime_error("AtomicController::normalizeSuffix: suffix cannot be only '_'");
    }

    return suffix;
}

std::string escapeRegexLiteral(const std::string &value) {
    std::string result{};

    for (const char item : value) {
        switch (item) {
            case '\\':
            case '^':
            case '$':
            case '.':
            case '|':
            case '?':
            case '*':
            case '+':
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
                result += '\\';
                result += item;
                break;
            default:
                result += item;
                break;
        }
    }

    return result;
}

} // namespace

AtomicController::AtomicController(controllers::EngineController &engine, AtomicControllerOptions options)
    : engine_{engine},
      options_{std::move(options)},
      literals_{},
      operations_{},
      literalIds_{},
      operationIds_{},
      registeredPatterns_{},
      unaryHandlers_{},
      binaryNodeInstalled_{},
      unaryNodeInstalled_{} {
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

AtomicController &AtomicController::integer(std::string nodeKind) {
    return use(literals::IntegerLiteralAtomic{std::move(nodeKind), TokenPattern::key("$int")});
}

AtomicController &AtomicController::integer(std::string nodeKind, std::vector<std::string> suffixes) {
    return integerSuffixRegex(suffixRegexFromList(suffixes), std::move(nodeKind));
}

AtomicController &AtomicController::integerSuffix(std::string suffix, std::string nodeKind) {
    return use(literals::IntegerLiteralAtomic{std::move(nodeKind), suffixPattern("$int", std::move(suffix))});
}

AtomicController &AtomicController::integerSuffixRegex(std::string suffixRegex, std::string nodeKind) {
    return use(literals::IntegerLiteralAtomic{std::move(nodeKind), suffixRegexPattern("$int", std::move(suffixRegex))});
}

AtomicController &AtomicController::floating(std::string nodeKind) {
    return use(literals::FloatLiteralAtomic{std::move(nodeKind), TokenPattern::key("$float")});
}

AtomicController &AtomicController::floating(std::string nodeKind, std::vector<std::string> suffixes) {
    return floatingSuffixRegex(suffixRegexFromList(suffixes), std::move(nodeKind));
}

AtomicController &AtomicController::floatingSuffix(std::string suffix, std::string nodeKind) {
    return use(literals::FloatLiteralAtomic{std::move(nodeKind), suffixPattern("$float", std::move(suffix))});
}

AtomicController &AtomicController::floatingSuffixRegex(std::string suffixRegex, std::string nodeKind) {
    return use(literals::FloatLiteralAtomic{std::move(nodeKind), suffixRegexPattern("$float", std::move(suffixRegex))});
}

AtomicController &AtomicController::stringLiteral(std::string nodeKind) {
    return use(literals::StringLiteralAtomic{std::move(nodeKind), TokenPattern::key("$string")});
}

AtomicController &AtomicController::stringLiteral(std::string nodeKind, std::vector<std::string> suffixes) {
    return stringSuffixRegex(suffixRegexFromList(suffixes), std::move(nodeKind));
}

AtomicController &AtomicController::stringSuffix(std::string suffix, std::string nodeKind) {
    return use(literals::StringLiteralAtomic{std::move(nodeKind), suffixPattern("$string", std::move(suffix))});
}

AtomicController &AtomicController::stringSuffixRegex(std::string suffixRegex, std::string nodeKind) {
    return use(literals::StringLiteralAtomic{std::move(nodeKind), suffixRegexPattern("$string", std::move(suffixRegex))});
}

AtomicController &AtomicController::boolean(std::string nodeKind, std::string trueToken, std::string falseToken) {
    return use(literals::BooleanLiteralAtomic{
        std::move(nodeKind),
        literals::BooleanLiteralTokens{std::move(trueToken), std::move(falseToken)}});
}

AtomicController &AtomicController::add(std::string token) {
    return own(std::make_unique<operations::AddOperationAtomic>(TokenPattern::text(std::move(token))));
}

AtomicController &AtomicController::subtract(std::string token) {
    return own(std::make_unique<operations::SubtractOperationAtomic>(tokenPattern(std::move(token))));
}

AtomicController &AtomicController::multiply(std::string token) {
    return own(std::make_unique<operations::MultiplyOperationAtomic>(tokenPattern(std::move(token))));
}

AtomicController &AtomicController::divide(std::string token) {
    return own(std::make_unique<operations::DivideOperationAtomic>(tokenPattern(std::move(token))));
}

AtomicController &AtomicController::modulo(std::string token) {
    return own(std::make_unique<operations::ModuloOperationAtomic>(tokenPattern(std::move(token))));
}

AtomicController &AtomicController::equal(std::string token) {
    return own(std::make_unique<operations::EqualOperationAtomic>(tokenPattern(std::move(token))));
}

AtomicController &AtomicController::notEqual(std::string token) {
    return own(std::make_unique<operations::NotEqualOperationAtomic>(tokenPattern(std::move(token))));
}

AtomicController &AtomicController::less(std::string token) {
    return own(std::make_unique<operations::LessOperationAtomic>(tokenPattern(std::move(token))));
}

AtomicController &AtomicController::lessEqual(std::string token) {
    return own(std::make_unique<operations::LessEqualOperationAtomic>(tokenPattern(std::move(token))));
}

AtomicController &AtomicController::greater(std::string token) {
    return own(std::make_unique<operations::GreaterOperationAtomic>(TokenPattern::text(std::move(token))));
}

AtomicController &AtomicController::greaterEqual(std::string token) {
    return own(std::make_unique<operations::GreaterEqualOperationAtomic>(tokenPattern(std::move(token))));
}

AtomicController &AtomicController::logicalAnd(std::string token) {
    return own(std::make_unique<operations::LogicalAndOperationAtomic>(tokenPattern(std::move(token))));
}

AtomicController &AtomicController::logicalOr(std::string token) {
    return own(std::make_unique<operations::LogicalOrOperationAtomic>(tokenPattern(std::move(token))));
}

AtomicController &AtomicController::logicalNot(std::string token) {
    return own(std::make_unique<operations::LogicalNotOperationAtomic>(tokenPattern(std::move(token))));
}

AtomicController &AtomicController::negate(std::string token) {
    return own(std::make_unique<operations::NumericNegateOperationAtomic>(tokenPattern(std::move(token))));
}

AtomicController &AtomicController::standardLiterals() {
    integer();
    floating();
    stringLiteral();
    boolean();

    return *this;
}

AtomicController &AtomicController::standardNumericOperations() {
    add();
    subtract();
    multiply();
    divide();
    modulo();
    negate();

    return *this;
}

AtomicController &AtomicController::standardComparisonOperations() {
    equal();
    notEqual();
    less();
    lessEqual();
    greater();
    greaterEqual();

    return *this;
}

AtomicController &AtomicController::standardLogicalOperations() {
    logicalAnd();
    logicalOr();
    logicalNot();

    return *this;
}

AtomicController &AtomicController::standardOperations() {
    standardNumericOperations();
    standardComparisonOperations();
    standardLogicalOperations();

    return *this;
}

AtomicController &AtomicController::standardCore() {
    standardLiterals();
    standardOperations();

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

TokenPattern AtomicController::tokenPattern(std::string token) {
    if (token.empty()) {
        throw std::runtime_error("AtomicController::tokenPattern: token cannot be empty");
    }

    const bool keyword{
        std::all_of(
            token.begin(),
            token.end(),
            [](unsigned char value) {
                return std::isalnum(value) != 0 || value == '_';
            })
    };

    if (keyword) {
        return TokenPattern::keywordText(std::move(token));
    }

    return TokenPattern::text(std::move(token));
}

TokenPattern AtomicController::suffixPattern(std::string tokenKey, std::string suffix) {
    return TokenPattern::suffixed(std::move(tokenKey), "_" + normalizeSuffix(std::move(suffix)));
}

TokenPattern AtomicController::suffixRegexPattern(std::string tokenKey, std::string suffixRegex) {
    if (suffixRegex.empty()) {
        throw std::runtime_error("AtomicController::suffixRegexPattern: suffix regex cannot be empty");
    }

    return TokenPattern::suffixRegex(std::move(tokenKey), std::move(suffixRegex));
}

std::string AtomicController::suffixRegexFromList(const std::vector<std::string> &suffixes) {
    if (suffixes.empty()) {
        throw std::runtime_error("AtomicController::suffixRegexFromList: suffix list cannot be empty");
    }

    std::string result{"_("};

    for (std::size_t index{}; index < suffixes.size(); ++index) {
        if (index != 0) {
            result += '|';
        }

        result += escapeRegexLiteral(normalizeSuffix(suffixes[index]));
    }

    result += ')';

    return result;
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

AtomicController &AtomicController::own(std::unique_ptr<LiteralFeature> feature) {
    if (!feature) {
        throw std::runtime_error("AtomicController::own: literal feature cannot be null");
    }

    use(*feature);
    ownedLiterals_.push_back(std::move(feature));

    return *this;
}

AtomicController &AtomicController::own(std::unique_ptr<OperationFeature> feature) {
    if (!feature) {
        throw std::runtime_error("AtomicController::own: operation feature cannot be null");
    }

    use(*feature);
    ownedOperations_.push_back(std::move(feature));

    return *this;
}

} // namespace novac::assets::atomic