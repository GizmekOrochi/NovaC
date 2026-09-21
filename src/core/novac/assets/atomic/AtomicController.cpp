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
#include <string_view>
#include <utility>

namespace novac::assets::atomic {

namespace detail {

struct RegexEscaper {
    static constexpr std::string_view specialCharacters{"\\^$.|?*+()[]{}"};

    static bool needsEscape(char value) {
        return specialCharacters.find(value) != std::string_view::npos;
    }

    static std::string literal(std::string_view value) {
        std::string result{};
        result.reserve(value.size() * 2);

        for (const char item : value) {
            if (needsEscape(item)) {
                result += '\\';
            }

            result += item;
        }

        return result;
    }
};

struct SuffixNormalizer {
    static std::string normalize(std::string suffix) {
        if (suffix.empty()) {
            throw std::runtime_error("SuffixNormalizer::normalize: suffix cannot be empty");
        }

        if (suffix.front() == '_') {
            suffix.erase(suffix.begin());
        }

        if (suffix.empty()) {
            throw std::runtime_error("SuffixNormalizer::normalize: suffix cannot be only '_'");
        }

        return suffix;
    }
};

struct TokenPatternFactory {
    static TokenPattern token(std::string token) {
        if (token.empty()) {
            throw std::runtime_error("TokenPatternFactory::token: token cannot be empty");
        }

        const bool keyword{std::all_of( token.begin(), token.end(), [](unsigned char value) { return std::isalnum(value) != 0 || value == '_'; })};

        if (keyword) {
            return TokenPattern::keywordText(std::move(token));
        }

        return TokenPattern::text(std::move(token));
    }

    static TokenPattern suffix(std::string tokenKey, std::string suffix) {
        return TokenPattern::suffixed(
            std::move(tokenKey), "_" + SuffixNormalizer::normalize(std::move(suffix)));
    }

    static TokenPattern suffixRegex(std::string tokenKey, std::string suffixRegex) {
        if (suffixRegex.empty()) {
            throw std::runtime_error("TokenPatternFactory::suffixRegex: suffix regex cannot be empty");
        }

        return TokenPattern::suffixRegex(std::move(tokenKey), std::move(suffixRegex));
    }
};

struct SuffixPatternBuilder {
    static std::string regexFromList(const std::vector<std::string> &suffixes) {
        if (suffixes.empty()) {
            throw std::runtime_error("SuffixPatternBuilder::regexFromList: suffix list cannot be empty");
        }

        std::string result{"_("};

        for (std::size_t index{}; index < suffixes.size(); ++index) {
            if (index != 0) {
                result += '|';
            }

            result += RegexEscaper::literal(SuffixNormalizer::normalize(suffixes[index]));
        }

        result += ')';

        return result;
    }

    static TokenPattern regex(std::string tokenKey, const std::vector<std::string> &suffixes) {
        return TokenPatternFactory::suffixRegex(
            std::move(tokenKey),
            regexFromList(suffixes));
    }
};

} // namespace detail

LiteralPack &LiteralPack::merge(LiteralPack pack) {
    for (auto &feature : pack.features) {
        features.push_back(std::move(feature));
    }

    return *this;
}

OperationPack &OperationPack::merge(OperationPack pack) {
    for (auto &feature : pack.features) {
        features.push_back(std::move(feature));
    }

    return *this;
}

AtomicController::AtomicController(controllers::EngineController &engine, AtomicControllerOptions options)
    : engine_{engine},
      options_{std::move(options)} {
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

AtomicController &AtomicController::use(LiteralPack pack) {
    for (auto &feature : pack.features) {
        own(std::move(feature));
    }

    return *this;
}

AtomicController &AtomicController::use(OperationPack pack) {
    for (auto &feature : pack.features) {
        own(std::move(feature));
    }

    return *this;
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

AtomicController &AtomicController::integer() {
    return use(literals::integer());
}

AtomicController &AtomicController::integer(std::string nodeKind, std::vector<std::string> suffixes) {
    return use(literals::integer(std::move(nodeKind), std::move(suffixes)));
}

AtomicController &AtomicController::floating() {
    return use(literals::floating());
}

AtomicController &AtomicController::floating(std::string nodeKind, std::vector<std::string> suffixes) {
    return use(literals::floating(std::move(nodeKind), std::move(suffixes)));
}

AtomicController &AtomicController::stringLiteral() {
    return use(literals::stringLiteral());
}

AtomicController &AtomicController::stringLiteral(std::string nodeKind, std::vector<std::string> suffixes) {
    return use(literals::stringLiteral(std::move(nodeKind), std::move(suffixes)));
}

AtomicController &AtomicController::boolean() {
    return use(literals::boolean());
}

AtomicController &AtomicController::boolean(std::string nodeKind, std::string trueToken, std::string falseToken) {
    return use(literals::boolean(std::move(nodeKind), std::move(trueToken), std::move(falseToken)));
}

AtomicController &AtomicController::add(std::string token) {
    OperationPack pack{};
    pack.add<operations::AddOperationAtomic>(detail::TokenPatternFactory::token(std::move(token)));
    return use(std::move(pack));
}

AtomicController &AtomicController::subtract(std::string token) {
    OperationPack pack{};
    pack.add<operations::SubtractOperationAtomic>(detail::TokenPatternFactory::token(std::move(token)));
    return use(std::move(pack));
}

AtomicController &AtomicController::standardLiterals() {
    return use(literals::standard());
}

AtomicController &AtomicController::standardNumericOperations() {
    return use(operations::numeric());
}

AtomicController &AtomicController::standardComparisonOperations() {
    return use(operations::comparison());
}

AtomicController &AtomicController::standardLogicalOperations() {
    return use(operations::logical());
}

AtomicController &AtomicController::standardOperations() {
    return use(operations::standard());
}

AtomicController &AtomicController::standardCore() {
    standardLiterals();
    standardOperations();

    return *this;
}

AtomicController &AtomicController::installStandardCore() {
    return standardCore();
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

bool AtomicController::hasCapability(const std::string &capability) const {
    return engine_.hasCapability(capability);
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

    for (const std::string &capability : info.capabilities) {
        if (capability.empty()) {
            throw std::runtime_error("AtomicController::validateLiteral: provided capability cannot be empty");
        }
    }

    for (const std::string &requirement : info.requiredCapabilities) {
        if (requirement.empty()) {
            throw std::runtime_error("AtomicController::validateLiteral: required capability cannot be empty");
        }
        if (!hasCapability(requirement)) {
            throw std::runtime_error("AtomicController::validateLiteral: missing required capability '" + requirement + "' for literal '" + info.id + "'");
        }
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

    for (const std::string &capability : info.capabilities) {
        if (capability.empty()) {
            throw std::runtime_error("AtomicController::validateOperation: provided capability cannot be empty");
        }
    }

    for (const std::string &requirement : info.requiredCapabilities) {
        if (requirement.empty()) {
            throw std::runtime_error("AtomicController::validateOperation: required capability cannot be empty");
        }
        if (!hasCapability(requirement)) {
            throw std::runtime_error("AtomicController::validateOperation: missing required capability '" + requirement + "' for operation '" + info.id + "'");
        }
    }
}

void AtomicController::rememberLiteral(LiteralInfo info) {
    literalIds_.insert(info.id);
    for (const std::string &capability : info.capabilities) {
        engine_.registerCapability(capability);
    }
    literals_.push_back(std::move(info));
}

void AtomicController::rememberOperation(OperationInfo info) {
    operationIds_.insert(info.id);
    for (const std::string &capability : info.capabilities) {
        engine_.registerCapability(capability);
    }
    operations_.push_back(std::move(info));
}

namespace literals {

LiteralPack integer(std::string nodeKind) {
    LiteralPack pack;
    pack.add<IntegerLiteralAtomic>(std::move(nodeKind), TokenPattern::key("$int"));
    return pack;
}

LiteralPack integer(std::string nodeKind, std::vector<std::string> suffixes) {
    LiteralPack pack;
    pack.add<IntegerLiteralAtomic>(
        std::move(nodeKind),
        detail::SuffixPatternBuilder::regex("$int", suffixes));
    return pack;
}

LiteralPack floating(std::string nodeKind) {
    LiteralPack pack;
    pack.add<FloatLiteralAtomic>(std::move(nodeKind), TokenPattern::key("$float"));
    return pack;
}

LiteralPack floating(std::string nodeKind, std::vector<std::string> suffixes) {
    LiteralPack pack;
    pack.add<FloatLiteralAtomic>(
        std::move(nodeKind),
        detail::SuffixPatternBuilder::regex("$float", suffixes));
    return pack;
}

LiteralPack stringLiteral(std::string nodeKind) {
    LiteralPack pack;
    pack.add<StringLiteralAtomic>(std::move(nodeKind), TokenPattern::key("$string"));
    return pack;
}

LiteralPack stringLiteral(std::string nodeKind, std::vector<std::string> suffixes) {
    LiteralPack pack;
    pack.add<StringLiteralAtomic>(
        std::move(nodeKind),
        detail::SuffixPatternBuilder::regex("$string", suffixes));
    return pack;
}

LiteralPack boolean(std::string nodeKind, std::string trueToken, std::string falseToken) {
    LiteralPack pack;
    pack.add<BooleanLiteralAtomic>(
        std::move(nodeKind),
        BooleanLiteralTokens{std::move(trueToken), std::move(falseToken)});
    return pack;
}

LiteralPack standard() {
    LiteralPack pack;

    pack.merge(integer());
    pack.merge(floating());
    pack.merge(stringLiteral());
    pack.merge(boolean());

    return pack;
}

}

namespace operations {

OperationPack numeric(NumericOperationOptions options) {
    OperationPack pack;

    pack.add<AddOperationAtomic>(detail::TokenPatternFactory::token(std::move(options.add)));
    pack.add<SubtractOperationAtomic>(detail::TokenPatternFactory::token(std::move(options.subtract)));
    pack.add<MultiplyOperationAtomic>(detail::TokenPatternFactory::token(std::move(options.multiply)));
    pack.add<DivideOperationAtomic>(detail::TokenPatternFactory::token(std::move(options.divide)));
    pack.add<ModuloOperationAtomic>(detail::TokenPatternFactory::token(std::move(options.modulo)));
    pack.add<NumericNegateOperationAtomic>(detail::TokenPatternFactory::token(std::move(options.negate)));

    return pack;
}

OperationPack comparison(ComparisonOperationOptions options) {
    OperationPack pack;

    pack.add<EqualOperationAtomic>(detail::TokenPatternFactory::token(std::move(options.equal)));
    pack.add<NotEqualOperationAtomic>(detail::TokenPatternFactory::token(std::move(options.notEqual)));
    pack.add<LessOperationAtomic>(detail::TokenPatternFactory::token(std::move(options.less)));
    pack.add<LessEqualOperationAtomic>(detail::TokenPatternFactory::token(std::move(options.lessEqual)));
    pack.add<GreaterOperationAtomic>(detail::TokenPatternFactory::token(std::move(options.greater)));
    pack.add<GreaterEqualOperationAtomic>(detail::TokenPatternFactory::token(std::move(options.greaterEqual)));

    return pack;
}

OperationPack logical(LogicalOperationOptions options) {
    OperationPack pack;

    pack.add<LogicalAndOperationAtomic>(detail::TokenPatternFactory::token(std::move(options.andToken)));
    pack.add<LogicalOrOperationAtomic>(detail::TokenPatternFactory::token(std::move(options.orToken)));
    pack.add<LogicalNotOperationAtomic>(detail::TokenPatternFactory::token(std::move(options.notToken)));

    return pack;
}

OperationPack standard() {
    OperationPack pack;

    pack.merge(numeric());
    pack.merge(comparison());
    pack.merge(logical());

    return pack;
}

}

} // namespace novac::assets::atomic