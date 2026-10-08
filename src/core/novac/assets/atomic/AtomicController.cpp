#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/assets/AssetTraits.hpp"

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

    /**
     * @brief Implements the `needsEscape` operation.
     *
     * @param value Value supplied for `value`.
     * @return Value produced by the operation.
     */
    static bool needsEscape(char value) {
        return specialCharacters.find(value) != std::string_view::npos;
    }

    /**
     * @brief Implements the `literal` operation.
     *
     * @param value Value supplied for `value`.
     * @return Value produced by the operation.
     */
    static std::string literal(std::string_view value) {
        std::string result{};
        result.reserve(value.size() * 2);

        for(const char item : value) {
            if(needsEscape(item))
                result += '\\';

            result += item;
        }

        return result;
    }
};

struct SuffixNormalizer {
    /**
     * @brief Implements the `normalize` operation.
     *
     * @param suffix Value supplied for `suffix`.
     * @return Value produced by the operation.
     */
    static std::string normalize(std::string suffix) {
        if(suffix.empty())
            throw std::runtime_error("SuffixNormalizer::normalize: suffix cannot be empty");

        if(suffix.front() == '_')
            suffix.erase(suffix.begin());

        if(suffix.empty())
            throw std::runtime_error("SuffixNormalizer::normalize: suffix cannot be only '_'");

        return suffix;
    }
};

struct TokenPatternFactory {
    /**
     * @brief Implements the `token` operation.
     *
     * @param token Value supplied for `token`.
     * @return Value produced by the operation.
     */
    static TokenPattern token(std::string token) {
        if(token.empty())
            throw std::runtime_error("TokenPatternFactory::token: token cannot be empty");

        const bool keyword{std::all_of( token.begin(), token.end(), [](unsigned char value) { return std::isalnum(value) != 0 || value == '_'; })};

        if(keyword)
            return TokenPattern::keywordText(std::move(token));

        return TokenPattern::text(std::move(token));
    }

    /**
     * @brief Implements the `suffix` operation.
     *
     * @param tokenKey Value supplied for `tokenKey`.
     * @param suffix Value supplied for `suffix`.
     * @return Value produced by the operation.
     */
    static TokenPattern suffix(std::string tokenKey, std::string suffix) {
        return TokenPattern::suffixed(std::move(tokenKey), "_" + SuffixNormalizer::normalize(std::move(suffix)));
    }

    /**
     * @brief Implements the `suffixRegex` operation.
     *
     * @param tokenKey Value supplied for `tokenKey`.
     * @param suffixRegex Value supplied for `suffixRegex`.
     * @return Value produced by the operation.
     */
    static TokenPattern suffixRegex(std::string tokenKey, std::string suffixRegex) {
        if(suffixRegex.empty())
            throw std::runtime_error("TokenPatternFactory::suffixRegex: suffix regex cannot be empty");

        return TokenPattern::suffixRegex(std::move(tokenKey), std::move(suffixRegex));
    }
};

struct SuffixPatternBuilder {
    /**
     * @brief Implements the `regexFromList` operation.
     *
     * @param suffixes Value supplied for `suffixes`.
     * @return Value produced by the operation.
     */
    static std::string regexFromList(const std::vector<std::string> &suffixes) {
        if(suffixes.empty())
            throw std::runtime_error("SuffixPatternBuilder::regexFromList: suffix list cannot be empty");

        std::string result{"_("};

        for(std::size_t index{}; index < suffixes.size(); ++index) {
            if(index != 0)
                result += '|';

            result += RegexEscaper::literal(SuffixNormalizer::normalize(suffixes[index]));
        }

        result += ')';

        return result;
    }

    /**
     * @brief Implements the `regex` operation.
     *
     * @param tokenKey Value supplied for `tokenKey`.
     * @param suffixes Value supplied for `suffixes`.
     * @return Value produced by the operation.
     */
    static TokenPattern regex(std::string tokenKey, const std::vector<std::string> &suffixes) {
        return TokenPatternFactory::suffixRegex(std::move(tokenKey), regexFromList(suffixes));
    }
};

} // namespace detail

/**
 * @brief Adds the supplied behavior through `merge`.
 *
 * @param pack Value supplied for `pack`.
 * @return Value produced by the operation.
 */
LiteralPack &LiteralPack::merge(LiteralPack pack) {
    for(auto &feature : pack.features)
        features.push_back(std::move(feature));

    return *this;
}

/**
 * @brief Adds the supplied behavior through `merge`.
 *
 * @param pack Value supplied for `pack`.
 * @return Value produced by the operation.
 */
OperationPack &OperationPack::merge(OperationPack pack) {
    for(auto &feature : pack.features)
        features.push_back(std::move(feature));

    return *this;
}

/**
 * @brief Constructs a `AtomicController` instance.
 *
 * @param engine Value supplied for `engine`.
 * @param options Value supplied for `options`.
 */
AtomicController::AtomicController(controllers::EngineController &engine, AtomicControllerOptions options)
    : engine_{engine}, options_{std::move(options)} {
    if(options_.expressionDomain.empty())
        throw std::runtime_error("AtomicController::AtomicController: expression domain cannot be empty");

    if(options_.binaryNodeKind.empty())
        throw std::runtime_error("AtomicController::AtomicController: binary node kind cannot be empty");

    if(options_.unaryNodeKind.empty())
        throw std::runtime_error("AtomicController::AtomicController: unary node kind cannot be empty");

    engine_.setStartDomain(options_.expressionDomain);
}

/**
 * @brief Adds the supplied behavior through `use`.
 *
 * @param feature Value supplied for `feature`.
 * @return Value produced by the operation.
 */
AtomicController &AtomicController::use(const LiteralFeature &feature) {
    LiteralInfo info{feature.info()};
    validateLiteral(info);

    const controllers::EngineController engineSnapshot{engine_.snapshot()};
    const auto literalsSnapshot{literals_};
    const auto operationsSnapshot{operations_};
    const auto literalIdsSnapshot{literalIds_};
    const auto operationIdsSnapshot{operationIds_};
    const auto registeredPatternsSnapshot{registeredPatterns_};
    const auto unaryHandlersSnapshot{*unaryHandlers_};
    const bool binaryNodeInstalledSnapshot{binaryNodeInstalled_};
    const bool unaryNodeInstalledSnapshot{unaryNodeInstalled_};
    const std::size_t ownedLiteralsSize{ownedLiterals_.size()};
    const std::size_t ownedOperationsSize{ownedOperations_.size()};

    try {
        feature.install(*this);
        rememberLiteral(std::move(info));
    }
    catch (...) {
        engine_.restore(engineSnapshot);
        literals_ = literalsSnapshot;
        operations_ = operationsSnapshot;
        literalIds_ = literalIdsSnapshot;
        operationIds_ = operationIdsSnapshot;
        registeredPatterns_ = registeredPatternsSnapshot;
        *unaryHandlers_ = unaryHandlersSnapshot;
        binaryNodeInstalled_ = binaryNodeInstalledSnapshot;
        unaryNodeInstalled_ = unaryNodeInstalledSnapshot;
        ownedLiterals_.resize(ownedLiteralsSize);
        ownedOperations_.resize(ownedOperationsSize);
        throw;
    }

    return *this;
}

/**
 * @brief Adds the supplied behavior through `use`.
 *
 * @param feature Value supplied for `feature`.
 * @return Value produced by the operation.
 */
AtomicController &AtomicController::use(const OperationFeature &feature) {
    OperationInfo info{feature.info()};
    validateOperation(info);

    const controllers::EngineController engineSnapshot{engine_.snapshot()};
    const auto literalsSnapshot{literals_};
    const auto operationsSnapshot{operations_};
    const auto literalIdsSnapshot{literalIds_};
    const auto operationIdsSnapshot{operationIds_};
    const auto registeredPatternsSnapshot{registeredPatterns_};
    const auto unaryHandlersSnapshot{*unaryHandlers_};
    const bool binaryNodeInstalledSnapshot{binaryNodeInstalled_};
    const bool unaryNodeInstalledSnapshot{unaryNodeInstalled_};
    const std::size_t ownedLiteralsSize{ownedLiterals_.size()};
    const std::size_t ownedOperationsSize{ownedOperations_.size()};

    try {
        feature.install(*this);
        rememberOperation(std::move(info));
    }
    catch (...) {
        engine_.restore(engineSnapshot);
        literals_ = literalsSnapshot;
        operations_ = operationsSnapshot;
        literalIds_ = literalIdsSnapshot;
        operationIds_ = operationIdsSnapshot;
        registeredPatterns_ = registeredPatternsSnapshot;
        *unaryHandlers_ = unaryHandlersSnapshot;
        binaryNodeInstalled_ = binaryNodeInstalledSnapshot;
        unaryNodeInstalled_ = unaryNodeInstalledSnapshot;
        ownedLiterals_.resize(ownedLiteralsSize);
        ownedOperations_.resize(ownedOperationsSize);
        throw;
    }

    return *this;
}

/**
 * @brief Adds the supplied behavior through `use`.
 *
 * @param pack Value supplied for `pack`.
 * @return Value produced by the operation.
 */
AtomicController &AtomicController::use(LiteralPack pack) {
    for(auto &feature : pack.features)
        own(std::move(feature));

    return *this;
}

/**
 * @brief Adds the supplied behavior through `use`.
 *
 * @param pack Value supplied for `pack`.
 * @return Value produced by the operation.
 */
AtomicController &AtomicController::use(OperationPack pack) {
    for(auto &feature : pack.features)
        own(std::move(feature));

    return *this;
}

/**
 * @brief Adds the supplied behavior through `own`.
 *
 * @param feature Value supplied for `feature`.
 * @return Value produced by the operation.
 */
AtomicController &AtomicController::own(std::unique_ptr<LiteralFeature> feature) {
    if(!feature)
        throw std::runtime_error("AtomicController::own: literal feature cannot be null");

    use(*feature);
    ownedLiterals_.push_back(std::move(feature));

    return *this;
}

/**
 * @brief Adds the supplied behavior through `own`.
 *
 * @param feature Value supplied for `feature`.
 * @return Value produced by the operation.
 */
AtomicController &AtomicController::own(std::unique_ptr<OperationFeature> feature) {
    if(!feature)
        throw std::runtime_error("AtomicController::own: operation feature cannot be null");

    use(*feature);
    ownedOperations_.push_back(std::move(feature));

    return *this;
}

/**
 * @brief Implements the `integer` operation.
 *
 * @return Value produced by the operation.
 */
AtomicController &AtomicController::integer() {
    return use(literals::integer());
}

/**
 * @brief Implements the `integer` operation.
 *
 * @param nodeKind Value supplied for `nodeKind`.
 * @param suffixes Value supplied for `suffixes`.
 * @return Value produced by the operation.
 */
AtomicController &AtomicController::integer(std::string nodeKind, std::vector<std::string> suffixes) {
    return use(literals::integer(std::move(nodeKind), std::move(suffixes)));
}

/**
 * @brief Implements the `floating` operation.
 *
 * @return Value produced by the operation.
 */
AtomicController &AtomicController::floating() {
    return use(literals::floating());
}

/**
 * @brief Implements the `floating` operation.
 *
 * @param nodeKind Value supplied for `nodeKind`.
 * @param suffixes Value supplied for `suffixes`.
 * @return Value produced by the operation.
 */
AtomicController &AtomicController::floating(std::string nodeKind, std::vector<std::string> suffixes) {
    return use(literals::floating(std::move(nodeKind), std::move(suffixes)));
}

/**
 * @brief Implements the `stringLiteral` operation.
 *
 * @return Value produced by the operation.
 */
AtomicController &AtomicController::stringLiteral() {
    return use(literals::stringLiteral());
}

/**
 * @brief Implements the `stringLiteral` operation.
 *
 * @param nodeKind Value supplied for `nodeKind`.
 * @param suffixes Value supplied for `suffixes`.
 * @return Value produced by the operation.
 */
AtomicController &AtomicController::stringLiteral(std::string nodeKind, std::vector<std::string> suffixes) {
    return use(literals::stringLiteral(std::move(nodeKind), std::move(suffixes)));
}

/**
 * @brief Implements the `boolean` operation.
 *
 * @return Value produced by the operation.
 */
AtomicController &AtomicController::boolean() {
    return use(literals::boolean());
}

/**
 * @brief Implements the `boolean` operation.
 *
 * @param nodeKind Value supplied for `nodeKind`.
 * @param trueToken Value supplied for `trueToken`.
 * @param falseToken Value supplied for `falseToken`.
 * @return Value produced by the operation.
 */
AtomicController &AtomicController::boolean(std::string nodeKind, std::string trueToken, std::string falseToken) {
    return use(literals::boolean(std::move(nodeKind), std::move(trueToken), std::move(falseToken)));
}

/**
 * @brief Adds data through `add`.
 *
 * @param token Value supplied for `token`.
 * @return Value produced by the operation.
 */
AtomicController &AtomicController::add(std::string token) {
    OperationPack pack{};
    pack.add<operations::AddOperationAtomic>(detail::TokenPatternFactory::token(std::move(token)));
    return use(std::move(pack));
}

/**
 * @brief Implements the `subtract` operation.
 *
 * @param token Value supplied for `token`.
 * @return Value produced by the operation.
 */
AtomicController &AtomicController::subtract(std::string token) {
    OperationPack pack{};
    pack.add<operations::SubtractOperationAtomic>(detail::TokenPatternFactory::token(std::move(token)));
    return use(std::move(pack));
}

/**
 * @brief Configures the standard behavior provided by `standardLiterals`.
 *
 * @return Value produced by the operation.
 */
AtomicController &AtomicController::standardLiterals() {
    return use(literals::standard());
}

/**
 * @brief Configures the standard behavior provided by `standardNumericOperations`.
 *
 * @return Value produced by the operation.
 */
AtomicController &AtomicController::standardNumericOperations() {
    return use(operations::numeric());
}

/**
 * @brief Configures the standard behavior provided by `standardComparisonOperations`.
 *
 * @return Value produced by the operation.
 */
AtomicController &AtomicController::standardComparisonOperations() {
    return use(operations::comparison());
}

/**
 * @brief Configures the standard behavior provided by `standardLogicalOperations`.
 *
 * @return Value produced by the operation.
 */
AtomicController &AtomicController::standardLogicalOperations() {
    return use(operations::logical());
}

/**
 * @brief Configures the standard behavior provided by `standardOperations`.
 *
 * @return Value produced by the operation.
 */
AtomicController &AtomicController::standardOperations() {
    return use(operations::standard());
}

/**
 * @brief Configures the standard behavior provided by `standardCore`.
 *
 * @return Value produced by the operation.
 */
AtomicController &AtomicController::standardCore() {
    standardLiterals();
    standardOperations();

    return *this;
}

/**
 * @brief Installs the behavior provided by `installStandardCore`.
 *
 * @return Value produced by the operation.
 */
AtomicController &AtomicController::installStandardCore() {
    return standardCore();
}

/**
 * @brief Returns the value exposed by `engine`.
 *
 * @return Value produced by the operation.
 */
controllers::EngineController &AtomicController::engine() {
    return engine_;
}

/**
 * @brief Returns the value exposed by `engine`.
 *
 * @return Value produced by the operation.
 */
const controllers::EngineController &AtomicController::engine() const {
    return engine_;
}

/**
 * @brief Implements the `expressionDomain` operation.
 *
 * @return Value produced by the operation.
 */
const std::string &AtomicController::expressionDomain() const {
    return options_.expressionDomain;
}

/**
 * @brief Implements the `binaryNodeKind` operation.
 *
 * @return Value produced by the operation.
 */
const std::string &AtomicController::binaryNodeKind() const {
    return options_.binaryNodeKind;
}

/**
 * @brief Implements the `unaryNodeKind` operation.
 *
 * @return Value produced by the operation.
 */
const std::string &AtomicController::unaryNodeKind() const {
    return options_.unaryNodeKind;
}

/**
 * @brief Checks the condition represented by `hasLiteral`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
bool AtomicController::hasLiteral(const std::string &id) const {
    return literalIds_.find(id) != literalIds_.end();
}

/**
 * @brief Checks the condition represented by `hasOperation`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
bool AtomicController::hasOperation(const std::string &id) const {
    return operationIds_.find(id) != operationIds_.end();
}

/**
 * @brief Checks the condition represented by `hasCapability`.
 *
 * @param capability Value supplied for `capability`.
 * @return Value produced by the operation.
 */
bool AtomicController::hasCapability(const std::string &capability) const {
    return engine_.hasCapability(capability);
}

/**
 * @brief Returns the value exposed by `literals`.
 *
 * @return Value produced by the operation.
 */
const std::vector<LiteralInfo> &AtomicController::literals() const {
    return literals_;
}

/**
 * @brief Returns the value exposed by `operations`.
 *
 * @return Value produced by the operation.
 */
const std::vector<OperationInfo> &AtomicController::operations() const {
    return operations_;
}

/**
 * @brief Registers data through `registerPattern`.
 *
 * @param pattern Value supplied for `pattern`.
 */
void AtomicController::registerPattern(const TokenPattern &pattern) {
    if(pattern.token.empty())
        return;

    const std::string key{pattern.keyword ? "keyword:" + pattern.token : "symbol:" + pattern.token};

    if(registeredPatterns_.find(key) != registeredPatterns_.end())
        return;

    if(pattern.keyword)
        engine_.keyword(pattern.token);
    else
        engine_.symbol(pattern.token);

    registeredPatterns_.insert(key);
}

/**
 * @brief Ensures the invariant required by `ensureBinaryExpressionNode`.
 */
void AtomicController::ensureBinaryExpressionNode() {
    if(binaryNodeInstalled_)
        return;

    engine_.node({
        .kind = options_.binaryNodeKind,
        .fields = {
            {.name = fields::Operation.value, .kind = ast::FieldKind::String, .required = true},
            {.name = fields::Left.value, .kind = ast::FieldKind::Node, .required = true, .allowedNodeTraits = {novac::assets::traits::Expression}},
            {.name = fields::Right.value, .kind = ast::FieldKind::Node, .required = true, .allowedNodeTraits = {novac::assets::traits::Expression}}
        },
        .traits = {novac::assets::traits::Expression},
        .doc = "Atomic binary expression carrier"
    });

    engine_.setBinaryNodeKind(options_.binaryNodeKind);
    binaryNodeInstalled_ = true;
}

/**
 * @brief Ensures the invariant required by `ensureUnaryExpressionNode`.
 */
void AtomicController::ensureUnaryExpressionNode() {
    if(unaryNodeInstalled_)
        return;

    engine_.node({
        .kind = options_.unaryNodeKind,
        .fields = {
            {.name = fields::Operation.value, .kind = ast::FieldKind::String, .required = true},
            {.name = fields::Expression.value, .kind = ast::FieldKind::Node, .required = true, .allowedNodeTraits = {novac::assets::traits::Expression}}
        },
        .traits = {novac::assets::traits::Expression},
        .doc = "Atomic unary expression carrier"
    });

    // The engine can outlive this controller. Share the handler storage instead
    // of capturing `this`, so dispatch survives controller moves/destruction.
    const auto handlers{unaryHandlers_};
    engine_.expression(options_.unaryNodeKind, [handlers](const ast::Node &node, runtime::RuntimeContext &context) {
        const std::string op{node.str(fields::Operation)};
        const auto iter{handlers->find(op)};

        if(iter == handlers->end())
            throw std::runtime_error("AtomicController::ensureUnaryExpressionNode: missing unary operation handler for '" + op + "'");

        return iter->second(node, context);
    });

    unaryNodeInstalled_ = true;
}

/**
 * @brief Registers data through `registerUnaryOperation`.
 *
 * @param operationId Value supplied for `operationId`.
 * @param handler Value supplied for `handler`.
 */
void AtomicController::registerUnaryOperation(std::string operationId, runtime::ExprHandler handler) {
    if(operationId.empty())
        throw std::runtime_error("AtomicController::registerUnaryOperation: operation id cannot be empty");

    if(!handler)
        throw std::runtime_error("AtomicController::registerUnaryOperation: handler cannot be empty");

    const auto result{unaryHandlers_->emplace(std::move(operationId), std::move(handler))};

    if(!result.second)
        throw std::runtime_error("AtomicController::registerUnaryOperation: duplicate unary operation handler");
}

/**
 * @brief Validates data through `validateLiteral`.
 *
 * @param info Value supplied for `info`.
 */
void AtomicController::validateLiteral(const LiteralInfo &info) const {
    if(info.id.empty())
        throw std::runtime_error("AtomicController::validateLiteral: literal id cannot be empty");

    if(info.nodeKind.empty())
        throw std::runtime_error("AtomicController::validateLiteral: literal node kind cannot be empty");

    if(info.pattern.token.empty() && info.pattern.tokenKey.empty())
        throw std::runtime_error("AtomicController::validateLiteral: literal pattern cannot be empty");

    if(hasLiteral(info.id))
        throw std::runtime_error("AtomicController::validateLiteral: duplicate literal '" + info.id + "'");

    for(const std::string &capability : info.capabilities) {
        if(capability.empty())
            throw std::runtime_error("AtomicController::validateLiteral: provided capability cannot be empty");
    }

    for(const std::string &requirement : info.requiredCapabilities) {
        if(requirement.empty())
            throw std::runtime_error("AtomicController::validateLiteral: required capability cannot be empty");
        if(!hasCapability(requirement))
            throw std::runtime_error("AtomicController::validateLiteral: missing required capability '" + requirement + "' for literal '" + info.id + "'");
    }
}

/**
 * @brief Validates data through `validateOperation`.
 *
 * @param info Value supplied for `info`.
 */
void AtomicController::validateOperation(const OperationInfo &info) const {
    if(info.id.empty())
        throw std::runtime_error("AtomicController::validateOperation: operation id cannot be empty");

    if(info.pattern.token.empty() && info.pattern.tokenKey.empty())
        throw std::runtime_error("AtomicController::validateOperation: operation pattern cannot be empty");

    if(hasOperation(info.id))
        throw std::runtime_error("AtomicController::validateOperation: duplicate operation '" + info.id + "'");

    for(const std::string &capability : info.capabilities)
        if(capability.empty())
            throw std::runtime_error("AtomicController::validateOperation: provided capability cannot be empty");

    for(const std::string &requirement : info.requiredCapabilities) {
        if(requirement.empty())
            throw std::runtime_error("AtomicController::validateOperation: required capability cannot be empty");
        if(!hasCapability(requirement))
            throw std::runtime_error("AtomicController::validateOperation: missing required capability '" + requirement + "' for operation '" + info.id + "'");
    }
}

/**
 * @brief Implements the `rememberLiteral` operation.
 *
 * @param info Value supplied for `info`.
 */
void AtomicController::rememberLiteral(LiteralInfo info) {
    literalIds_.insert(info.id);
    for(const std::string &capability : info.capabilities)
        engine_.registerCapability(capability);
    literals_.push_back(std::move(info));
}

/**
 * @brief Implements the `rememberOperation` operation.
 *
 * @param info Value supplied for `info`.
 */
void AtomicController::rememberOperation(OperationInfo info) {
    operationIds_.insert(info.id);
    for(const std::string &capability : info.capabilities)
        engine_.registerCapability(capability);
    operations_.push_back(std::move(info));
}

namespace literals {

/**
 * @brief Implements the `integer` operation.
 *
 * @param nodeKind Value supplied for `nodeKind`.
 * @return Value produced by the operation.
 */
LiteralPack integer(std::string nodeKind) {
    LiteralPack pack;
    pack.add<IntegerLiteralAtomic>(std::move(nodeKind), TokenPattern::key("$int"));
    return pack;
}

/**
 * @brief Implements the `integer` operation.
 *
 * @param nodeKind Value supplied for `nodeKind`.
 * @param suffixes Value supplied for `suffixes`.
 * @return Value produced by the operation.
 */
LiteralPack integer(std::string nodeKind, std::vector<std::string> suffixes) {
    LiteralPack pack;
    pack.add<IntegerLiteralAtomic>(std::move(nodeKind), detail::SuffixPatternBuilder::regex("$int", suffixes));
    return pack;
}

/**
 * @brief Implements the `floating` operation.
 *
 * @param nodeKind Value supplied for `nodeKind`.
 * @return Value produced by the operation.
 */
LiteralPack floating(std::string nodeKind) {
    LiteralPack pack;
    pack.add<FloatLiteralAtomic>(std::move(nodeKind), TokenPattern::key("$float"));
    return pack;
}

/**
 * @brief Implements the `floating` operation.
 *
 * @param nodeKind Value supplied for `nodeKind`.
 * @param suffixes Value supplied for `suffixes`.
 * @return Value produced by the operation.
 */
LiteralPack floating(std::string nodeKind, std::vector<std::string> suffixes) {
    LiteralPack pack;
    pack.add<FloatLiteralAtomic>(std::move(nodeKind), detail::SuffixPatternBuilder::regex("$float", suffixes));
    return pack;
}

/**
 * @brief Implements the `stringLiteral` operation.
 *
 * @param nodeKind Value supplied for `nodeKind`.
 * @return Value produced by the operation.
 */
LiteralPack stringLiteral(std::string nodeKind) {
    LiteralPack pack;
    pack.add<StringLiteralAtomic>(std::move(nodeKind), TokenPattern::key("$string"));
    return pack;
}

/**
 * @brief Implements the `stringLiteral` operation.
 *
 * @param nodeKind Value supplied for `nodeKind`.
 * @param suffixes Value supplied for `suffixes`.
 * @return Value produced by the operation.
 */
LiteralPack stringLiteral(std::string nodeKind, std::vector<std::string> suffixes) {
    LiteralPack pack;
    pack.add<StringLiteralAtomic>(std::move(nodeKind), detail::SuffixPatternBuilder::regex("$string", suffixes));
    return pack;
}

/**
 * @brief Implements the `boolean` operation.
 *
 * @param nodeKind Value supplied for `nodeKind`.
 * @param trueToken Value supplied for `trueToken`.
 * @param falseToken Value supplied for `falseToken`.
 * @return Value produced by the operation.
 */
LiteralPack boolean(std::string nodeKind, std::string trueToken, std::string falseToken) {
    LiteralPack pack;
    pack.add<BooleanLiteralAtomic>(std::move(nodeKind), BooleanLiteralTokens{std::move(trueToken), std::move(falseToken)});
    return pack;
}

/**
 * @brief Configures the standard behavior provided by `standard`.
 *
 * @return Value produced by the operation.
 */
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

/**
 * @brief Implements the `numeric` operation.
 *
 * @param options Value supplied for `options`.
 * @return Value produced by the operation.
 */
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

/**
 * @brief Implements the `comparison` operation.
 *
 * @param options Value supplied for `options`.
 * @return Value produced by the operation.
 */
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

/**
 * @brief Implements the `logical` operation.
 *
 * @param options Value supplied for `options`.
 * @return Value produced by the operation.
 */
OperationPack logical(LogicalOperationOptions options) {
    OperationPack pack;

    pack.add<LogicalAndOperationAtomic>(detail::TokenPatternFactory::token(std::move(options.andToken)));
    pack.add<LogicalOrOperationAtomic>(detail::TokenPatternFactory::token(std::move(options.orToken)));
    pack.add<LogicalNotOperationAtomic>(detail::TokenPatternFactory::token(std::move(options.notToken)));

    return pack;
}

/**
 * @brief Configures the standard behavior provided by `standard`.
 *
 * @return Value produced by the operation.
 */
OperationPack standard() {
    OperationPack pack;

    pack.merge(numeric());
    pack.merge(comparison());
    pack.merge(logical());

    return pack;
}

}

} // namespace novac::assets::atomic
