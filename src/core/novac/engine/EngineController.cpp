#include "novac/engine/EngineController.hpp"

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace novac::controllers {

/**
 * @brief Constructs a `EngineFeature` instance.
 *
 * @param name Value supplied for `name`.
 */
EngineFeature::EngineFeature(std::string name)
    : name_{std::move(name)},
      version_{"0.1.0"},
      description_{},
      capabilities_{},
      requiredCapabilities_{},
      conflicts_{},
      installers_{} {
    if (name_.empty()) {
        throw std::runtime_error("EngineFeature::EngineFeature: feature name cannot be empty");
    }
}

/**
 * @brief Implements the `version` operation.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
EngineFeature &EngineFeature::version(std::string value) {
    if (value.empty()) {
        throw std::runtime_error("EngineFeature::version: version cannot be empty");
    }

    version_ = std::move(value);

    return *this;
}

/**
 * @brief Implements the `description` operation.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
EngineFeature &EngineFeature::description(std::string value) {
    description_ = std::move(value);

    return *this;
}

/**
 * @brief Implements the `provides` operation.
 *
 * @param capability Value supplied for `capability`.
 * @return Value produced by the operation.
 */
EngineFeature &EngineFeature::provides(std::string capability) {
    if (capability.empty()) {
        throw std::runtime_error("EngineFeature::provides: capability cannot be empty");
    }

    capabilities_.push_back(std::move(capability));

    return *this;
}

/**
 * @brief Returns the value required by `requiresCapability`.
 *
 * @param capability Value supplied for `capability`.
 * @return Value produced by the operation.
 */
EngineFeature &EngineFeature::requiresCapability(std::string capability) {
    if (capability.empty()) {
        throw std::runtime_error("EngineFeature::requiresCapability: capability cannot be empty");
    }

    requiredCapabilities_.push_back(std::move(capability));

    return *this;
}

/**
 * @brief Implements the `dependsOn` operation.
 *
 * @param capability Value supplied for `capability`.
 * @return Value produced by the operation.
 */
EngineFeature &EngineFeature::dependsOn(std::string capability) {
    return requiresCapability(std::move(capability));
}

/**
 * @brief Implements the `conflictsWith` operation.
 *
 * @param featureName Value supplied for `featureName`.
 * @return Value produced by the operation.
 */
EngineFeature &EngineFeature::conflictsWith(std::string featureName) {
    if (featureName.empty()) {
        throw std::runtime_error("EngineFeature::conflictsWith: feature name cannot be empty");
    }

    conflicts_.push_back(std::move(featureName));

    return *this;
}

/**
 * @brief Implements the `onInstall` operation.
 *
 * @param installer Value supplied for `installer`.
 * @return Value produced by the operation.
 */
EngineFeature &EngineFeature::onInstall(Installer installer) {
    if (!installer) {
        throw std::runtime_error("EngineFeature::onInstall: installer cannot be empty");
    }

    installers_.push_back(std::move(installer));

    return *this;
}

/**
 * @brief Returns the value exposed by `name`.
 *
 * @return Value produced by the operation.
 */
const std::string &EngineFeature::name() const {
    return name_;
}

/**
 * @brief Implements the `version` operation.
 *
 * @return Value produced by the operation.
 */
const std::string &EngineFeature::version() const {
    return version_;
}

/**
 * @brief Implements the `description` operation.
 *
 * @return Value produced by the operation.
 */
const std::string &EngineFeature::description() const {
    return description_;
}

/**
 * @brief Returns the value exposed by `capabilities`.
 *
 * @return Value produced by the operation.
 */
const std::vector<std::string> &EngineFeature::capabilities() const {
    return capabilities_;
}

/**
 * @brief Returns the value required by `requiredCapabilities`.
 *
 * @return Value produced by the operation.
 */
const std::vector<std::string> &EngineFeature::requiredCapabilities() const {
    return requiredCapabilities_;
}

/**
 * @brief Implements the `conflicts` operation.
 *
 * @return Value produced by the operation.
 */
const std::vector<std::string> &EngineFeature::conflicts() const {
    return conflicts_;
}

/**
 * @brief Installs the behavior provided by `install`.
 *
 * @param engine Value supplied for `engine`.
 */
void EngineFeature::install(EngineController &engine) const {
    for (const Installer &installer : installers_) {
        installer(engine);
    }
}

/**
 * @brief Constructs a `EngineController` instance.
 *
 * @param options Value supplied for `options`.
 */
EngineController::EngineController(EngineControllerOptions options)
    : lexer_{options.duplicatePolicy},
      nodes_{options.duplicatePolicy},
      parser_{options.duplicatePolicy},
      runtime_{options.duplicatePolicy},
      sources_{options.duplicatePolicy},
      preprocessor_{options.duplicatePolicy},
      lowering_{options.duplicatePolicy},
      diagnostics_{},
      startDomain_{std::move(options.startDomain)},
      features_{},
      capabilities_{} {
}

/**
 * @brief Implements the `keyword` operation.
 *
 * @param keyword Value supplied for `keyword`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::keyword(std::string keyword) {
    return lexer_.keyword(std::move(keyword));
}

/**
 * @brief Implements the `symbol` operation.
 *
 * @param symbol Value supplied for `symbol`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::symbol(std::string symbol) {
    return lexer_.symbol(std::move(symbol));
}

/**
 * @brief Implements the `directive` operation.
 *
 * @param name Value supplied for `name`.
 * @param handler Value supplied for `handler`.
 * @param mode Value supplied for `mode`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::directive(
    std::string name,
    source::PreprocessorController::DirectiveHandler handler,
    source::DirectiveMode mode) {
    return preprocessor_.directive(std::move(name), std::move(handler), mode);
}

/**
 * @brief Implements the `pragma` operation.
 *
 * @param name Value supplied for `name`.
 * @param handler Value supplied for `handler`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::pragma(
    std::string name,
    source::PreprocessorController::PragmaHandler handler) {
    return preprocessor_.pragma(std::move(name), std::move(handler));
}

/**
 * @brief Implements the `node` operation.
 *
 * @param schema Value supplied for `schema`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::node(ast::NodeSchema schema) {
    return nodes_.registerNode(std::move(schema));
}

/**
 * @brief Parses input through `parseRule`.
 *
 * @param domain Value supplied for `domain`.
 * @param key Value supplied for `key`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::parseRule(std::string domain, std::string key, parser::ParseFn fn) {
    return parser_.rule(std::move(domain), std::move(key), std::move(fn));
}

/**
 * @brief Parses input through `parseRule`.
 *
 * @param domain Value supplied for `domain`.
 * @param key Value supplied for `key`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::parseRule(const ids::ParseDomain &domain, std::string key, parser::ParseFn fn) {
    return parser_.rule(domain, std::move(key), std::move(fn));
}

/**
 * @brief Implements the `fallback` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::fallback(std::string domain, parser::ParseFn fn) {
    return parser_.fallback(std::move(domain), std::move(fn));
}

/**
 * @brief Implements the `fallback` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::fallback(const ids::ParseDomain &domain, parser::ParseFn fn) {
    return parser_.fallback(domain, std::move(fn));
}

/**
 * @brief Implements the `prefix` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param key Value supplied for `key`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::prefix(std::string domain, std::string key, parser::PrefixFn fn) {
    return parser_.prefix(std::move(domain), std::move(key), std::move(fn));
}

/**
 * @brief Implements the `prefix` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param key Value supplied for `key`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::prefix(const ids::ParseDomain &domain, std::string key, parser::PrefixFn fn) {
    return parser_.prefix(domain, std::move(key), std::move(fn));
}

/**
 * @brief Implements the `prefixFallback` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param key Value supplied for `key`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::prefixFallback(std::string domain, std::string key, parser::PrefixFn fn) {
    return parser_.prefixFallback(std::move(domain), std::move(key), std::move(fn));
}

/**
 * @brief Implements the `prefixFallback` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param key Value supplied for `key`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::prefixFallback(const ids::ParseDomain &domain, std::string key, parser::PrefixFn fn) {
    return parser_.prefixFallback(domain, std::move(key), std::move(fn));
}

/**
 * @brief Implements the `infix` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param op Value supplied for `op`.
 * @param precedence Value supplied for `precedence`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::infix(std::string domain, std::string op, int precedence, parser::InfixFn fn) {
    return parser_.infix(std::move(domain), std::move(op), precedence, std::move(fn));
}

/**
 * @brief Implements the `infix` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param op Value supplied for `op`.
 * @param precedence Value supplied for `precedence`.
 * @param associativity Value supplied for `associativity`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::infix(std::string domain, std::string op, int precedence, parser::Associativity associativity, parser::InfixFn fn) {
    return parser_.infix(std::move(domain), std::move(op), precedence, associativity, std::move(fn));
}

/**
 * @brief Implements the `infix` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param op Value supplied for `op`.
 * @param precedence Value supplied for `precedence`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::infix(const ids::ParseDomain &domain, std::string op, int precedence, parser::InfixFn fn) {
    return parser_.infix(domain, std::move(op), precedence, std::move(fn));
}

/**
 * @brief Implements the `infix` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param op Value supplied for `op`.
 * @param precedence Value supplied for `precedence`.
 * @param associativity Value supplied for `associativity`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::infix(const ids::ParseDomain &domain, std::string op, int precedence, parser::Associativity associativity, parser::InfixFn fn) {
    return parser_.infix(domain, std::move(op), precedence, associativity, std::move(fn));
}

/**
 * @brief Implements the `postfix` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param op Value supplied for `op`.
 * @param precedence Value supplied for `precedence`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::postfix(std::string domain, std::string op, int precedence, parser::PostfixFn fn) {
    return parser_.postfix(std::move(domain), std::move(op), precedence, std::move(fn));
}

/**
 * @brief Implements the `postfix` operation.
 *
 * @param domain Value supplied for `domain`.
 * @param op Value supplied for `op`.
 * @param precedence Value supplied for `precedence`.
 * @param fn Value supplied for `fn`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::postfix(const ids::ParseDomain &domain, std::string op, int precedence, parser::PostfixFn fn) {
    return parser_.postfix(domain, std::move(op), precedence, std::move(fn));
}

/**
 * @brief Implements the `expression` operation.
 *
 * @param kind Value supplied for `kind`.
 * @param handler Value supplied for `handler`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::expression(std::string kind, runtime::ExprHandler handler) {
    return runtime_.expression(std::move(kind), std::move(handler));
}

/**
 * @brief Implements the `expression` operation.
 *
 * @param kind Value supplied for `kind`.
 * @param handler Value supplied for `handler`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::expression(const ids::NodeKind &kind, runtime::ExprHandler handler) {
    return runtime_.expression(kind, std::move(handler));
}

/**
 * @brief Implements the `statement` operation.
 *
 * @param kind Value supplied for `kind`.
 * @param handler Value supplied for `handler`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::statement(std::string kind, runtime::StmtHandler handler) {
    return runtime_.statement(std::move(kind), std::move(handler));
}

/**
 * @brief Implements the `statement` operation.
 *
 * @param kind Value supplied for `kind`.
 * @param handler Value supplied for `handler`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::statement(const ids::NodeKind &kind, runtime::StmtHandler handler) {
    return runtime_.statement(kind, std::move(handler));
}

/**
 * @brief Implements the `declaration` operation.
 *
 * @param kind Value supplied for `kind`.
 * @param handler Value supplied for `handler`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::declaration(std::string kind, runtime::DeclHandler handler) {
    return runtime_.declaration(std::move(kind), std::move(handler));
}

/**
 * @brief Implements the `declaration` operation.
 *
 * @param kind Value supplied for `kind`.
 * @param handler Value supplied for `handler`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::declaration(const ids::NodeKind &kind, runtime::DeclHandler handler) {
    return runtime_.declaration(kind, std::move(handler));
}

/**
 * @brief Implements the `binaryOperator` operation.
 *
 * @param op Value supplied for `op`.
 * @param handler Value supplied for `handler`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::binaryOperator(std::string op, runtime::BinaryHandler handler) {
    return runtime_.binaryOperator(std::move(op), std::move(handler));
}

/**
 * @brief Implements the `binaryOperator` operation.
 *
 * @param op Value supplied for `op`.
 * @param handler Value supplied for `handler`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::binaryOperator(const ids::Operation &op, runtime::BinaryHandler handler) {
    return runtime_.binaryOperator(op, std::move(handler));
}

/**
 * @brief Sets the value handled by `setBinaryNodeKind`.
 *
 * @param kind Value supplied for `kind`.
 */
void EngineController::setBinaryNodeKind(std::string kind) {
    runtime_.setBinaryNodeKind(std::move(kind));
}

/**
 * @brief Implements the `hir` operation.
 *
 * @param nodeKind Value supplied for `nodeKind`.
 * @param lowerer Value supplied for `lowerer`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::hir(std::string nodeKind, ir::LoweringRegistry::HIRLowerer lowerer) {
    return lowering_.hir(std::move(nodeKind), std::move(lowerer));
}

/**
 * @brief Implements the `hir` operation.
 *
 * @param nodeKind Value supplied for `nodeKind`.
 * @param lowerer Value supplied for `lowerer`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::hir(const ids::NodeKind &nodeKind, ir::LoweringRegistry::HIRLowerer lowerer) {
    return lowering_.hir(nodeKind, std::move(lowerer));
}

/**
 * @brief Implements the `mir` operation.
 *
 * @param instructionKind Value supplied for `instructionKind`.
 * @param lowerer Value supplied for `lowerer`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::mir(std::string instructionKind, ir::LoweringRegistry::MIRLowerer lowerer) {
    return lowering_.mir(std::move(instructionKind), std::move(lowerer));
}

/**
 * @brief Implements the `mir` operation.
 *
 * @param instructionKind Value supplied for `instructionKind`.
 * @param lowerer Value supplied for `lowerer`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus EngineController::mir(const ids::Operation &instructionKind, ir::LoweringRegistry::MIRLowerer lowerer) {
    return lowering_.mir(instructionKind, std::move(lowerer));
}

/**
 * @brief Creates a value through `makeNode`.
 *
 * @param kind Value supplied for `kind`.
 * @return Value produced by the operation.
 */
ast::NodePtr EngineController::makeNode(std::string kind) const {
    return ast::Node::make(std::move(kind));
}

/**
 * @brief Creates a value through `makeNode`.
 *
 * @param kind Value supplied for `kind`.
 * @return Value produced by the operation.
 */
ast::NodePtr EngineController::makeNode(const ids::NodeKind &kind) const {
    return ast::Node::make(kind);
}

/**
 * @brief Implements the `tokenize` operation.
 *
 * @param source Value supplied for `source`.
 * @return Value produced by the operation.
 */
std::vector<token::Token> EngineController::tokenize(const std::string &source) const {
    lexer::Lexer lexer{lexer_};

    try {
        return lexer.tokenize(source);
    } catch (const std::runtime_error &error) {
        diagnostics_.error(error.what());
        throw;
    }
}

/**
 * @brief Implements the `preprocess` operation.
 *
 * @param sourceValue Value supplied for `sourceValue`.
 * @param options Value supplied for `options`.
 * @return Value produced by the operation.
 */
source::PreprocessedSource EngineController::preprocess(
    const source::Source &sourceValue,
    source::PreprocessOptions options) const {
    try {
        return preprocessor_.process(sourceValue, sources_, lexer_, diagnostics_, std::move(options));
    } catch (const std::runtime_error &error) {
        diagnostics_.error(error.what());
        throw;
    }
}

/**
 * @brief Implements the `tokenizeSource` operation.
 *
 * @param sourceValue Value supplied for `sourceValue`.
 * @param options Value supplied for `options`.
 * @return Value produced by the operation.
 */
std::vector<token::Token> EngineController::tokenizeSource(
    const source::Source &sourceValue,
    source::PreprocessOptions options) const {
    const source::PreprocessedSource processed{preprocess(sourceValue, std::move(options))};
    lexer::Lexer lexer{lexer_};
    std::vector<token::Token> tokens{};

    try {
        for (const source::SourceFragment &fragment : processed.fragments) {
            std::vector<token::Token> fragmentTokens{lexer.tokenize(fragment.text, fragment.origin)};
            if (!fragmentTokens.empty()) {
                fragmentTokens.pop_back();
            }
            tokens.insert(
                tokens.end(),
                std::make_move_iterator(fragmentTokens.begin()),
                std::make_move_iterator(fragmentTokens.end()));
        }
    } catch (const std::runtime_error &error) {
        diagnostics_.error(error.what());
        throw;
    }

    const diagnostics::SourceLocation &endLocation{processed.terminalLocation};
    tokens.push_back({token::Kind::End, "", "", {endLocation, endLocation}, endLocation.line, endLocation.column});
    return tokens;
}

/**
 * @brief Parses input through `parseSource`.
 *
 * @param sourceValue Value supplied for `sourceValue`.
 * @param options Value supplied for `options`.
 * @return Value produced by the operation.
 */
ast::NodePtr EngineController::parseSource(
    const source::Source &sourceValue,
    source::PreprocessOptions options) const {
    requireStartDomain("EngineController::parseSource");
    return parseTokens(tokenizeSource(sourceValue, std::move(options)), startDomain_);
}

/**
 * @brief Parses input through `parseSource`.
 *
 * @param specifier Value supplied for `specifier`.
 * @param options Value supplied for `options`.
 * @return Value produced by the operation.
 */
ast::NodePtr EngineController::parseSource(
    const std::string &specifier,
    source::PreprocessOptions options) const {
    source::Source resolved{};
    try {
        resolved = sources_.resolve({specifier, {}});
    } catch (const std::runtime_error &error) {
        diagnostics_.error(error.what());
        throw;
    }
    return parseSource(resolved, std::move(options));
}

/**
 * @brief Parses input through `parse`.
 *
 * @param source Value supplied for `source`.
 * @return Value produced by the operation.
 */
ast::NodePtr EngineController::parse(const std::string &source) const {
    requireStartDomain("EngineController::parse");

    return parse(source, startDomain_);
}

/**
 * @brief Parses input through `parse`.
 *
 * @param source Value supplied for `source`.
 * @param startDomain Value supplied for `startDomain`.
 * @return Value produced by the operation.
 */
ast::NodePtr EngineController::parse(const std::string &source, std::string startDomain) const {
    return parseTokens(tokenize(source), std::move(startDomain));
}

/**
 * @brief Parses input through `parseTokens`.
 *
 * @param tokens Value supplied for `tokens`.
 * @return Value produced by the operation.
 */
ast::NodePtr EngineController::parseTokens(std::vector<token::Token> tokens) const {
    requireStartDomain("EngineController::parseTokens");

    return parseTokens(std::move(tokens), startDomain_);
}

/**
 * @brief Parses input through `parseTokens`.
 *
 * @param tokens Value supplied for `tokens`.
 * @param startDomain Value supplied for `startDomain`.
 * @return Value produced by the operation.
 */
ast::NodePtr EngineController::parseTokens(std::vector<token::Token> tokens, std::string startDomain) const {
    if (startDomain.empty()) {
        const std::string message{"EngineController::parseTokens: start domain cannot be empty"};
        diagnostics_.error(message);
        throw std::runtime_error(message);
    }

    parser::Parser parser{parser_, std::move(startDomain)};

    try {
        return parser.parse(std::move(tokens));
    } catch (const std::runtime_error &error) {
        diagnostics_.error(error.what());
        throw;
    }
}

/**
 * @brief Validates data through `validate`.
 *
 * @param node Value supplied for `node`.
 */
void EngineController::validate(const ast::Node &node) const {
    try {
        nodes_.validate(node);
    } catch (const std::runtime_error &error) {
        diagnostics_.error(error.what());
        throw;
    }
}

/**
 * @brief Implements the `eval` operation.
 *
 * @param node Value supplied for `node`.
 * @return Value produced by the operation.
 */
runtime::Value EngineController::eval(const ast::Node &node) const {
    runtime::Runtime runtime{runtime_};

    try {
        return runtime.eval(node);
    } catch (const std::runtime_error &error) {
        diagnostics_.error(error.what());
        throw;
    }
}

/**
 * @brief Implements the `exec` operation.
 *
 * @param node Value supplied for `node`.
 */
void EngineController::exec(const ast::Node &node) const {
    runtime::Runtime runtime{runtime_};

    try {
        runtime.exec(node);
    } catch (const std::runtime_error &error) {
        diagnostics_.error(error.what());
        throw;
    }
}

/**
 * @brief Lowers input through `lowerToHIR`.
 *
 * @param node Value supplied for `node`.
 * @return Value produced by the operation.
 */
ir::HIRModule EngineController::lowerToHIR(const ast::Node &node) const {
    ir::ASTLoweringPass pass{lowering_};

    try {
        return pass.lower(node);
    } catch (const std::runtime_error &error) {
        diagnostics_.error(error.what());
        throw;
    }
}

/**
 * @brief Lowers input through `lowerToMIR`.
 *
 * @param hir Value supplied for `hir`.
 * @return Value produced by the operation.
 */
ir::MIRModule EngineController::lowerToMIR(const ir::HIRModule &hir) const {
    ir::HIRLoweringPass pass{lowering_};

    try {
        return pass.lower(hir);
    } catch (const std::runtime_error &error) {
        diagnostics_.error(error.what());
        throw;
    }
}

/**
 * @brief Lowers input through `lowerToMIR`.
 *
 * @param node Value supplied for `node`.
 * @return Value produced by the operation.
 */
ir::MIRModule EngineController::lowerToMIR(const ast::Node &node) const {
    ir::HIRModule hir{lowerToHIR(node)};

    return lowerToMIR(hir);
}

/**
 * @brief Installs the behavior provided by `install`.
 *
 * @param feature Value supplied for `feature`.
 */
void EngineController::install(const EngineFeature &feature) {
    validateFeatureInstall(feature);

    EngineController candidate{*this};
    feature.install(candidate);
    candidate.rememberFeature(feature);

    *this = std::move(candidate);
}

/**
 * @brief Implements the `snapshot` operation.
 *
 * @return Value produced by the operation.
 */
EngineController EngineController::snapshot() const {
    return *this;
}

/**
 * @brief Implements the `restore` operation.
 *
 * @param snapshot Value supplied for `snapshot`.
 */
void EngineController::restore(const EngineController &snapshot) {
    *this = snapshot;
}

/**
 * @brief Checks the condition represented by `hasFeature`.
 *
 * @param name Value supplied for `name`.
 * @return Value produced by the operation.
 */
bool EngineController::hasFeature(const std::string &name) const {
    return std::any_of(
        features_.begin(),
        features_.end(),
        [&](const InstalledFeature &feature) {
            return feature.name == name;
        });
}

/**
 * @brief Checks the condition represented by `hasCapability`.
 *
 * @param capability Value supplied for `capability`.
 * @return Value produced by the operation.
 */
bool EngineController::hasCapability(const std::string &capability) const {
    return capabilities_.find(capability) != capabilities_.end();
}

/**
 * @brief Registers data through `registerCapability`.
 *
 * @param capability Value supplied for `capability`.
 */
void EngineController::registerCapability(std::string capability) {
    if (capability.empty()) {
        throw std::runtime_error("EngineController::registerCapability: capability cannot be empty");
    }

    capabilities_.insert(std::move(capability));
}

/**
 * @brief Sets the value handled by `setStartDomain`.
 *
 * @param startDomain Value supplied for `startDomain`.
 */
void EngineController::setStartDomain(std::string startDomain) {
    if (startDomain.empty()) {
        throw std::runtime_error("EngineController::setStartDomain: start domain cannot be empty");
    }

    startDomain_ = std::move(startDomain);
}

/**
 * @brief Starts the operation represented by `startDomain`.
 *
 * @return Value produced by the operation.
 */
const std::string &EngineController::startDomain() const {
    return startDomain_;
}

/**
 * @brief Implements the `diagnostics` operation.
 *
 * @return Value produced by the operation.
 */
diagnostics::DiagnosticEngine &EngineController::diagnostics() {
    return diagnostics_;
}

/**
 * @brief Implements the `diagnostics` operation.
 *
 * @return Value produced by the operation.
 */
const diagnostics::DiagnosticEngine &EngineController::diagnostics() const {
    return diagnostics_;
}

/**
 * @brief Implements the `lexer` operation.
 *
 * @return Value produced by the operation.
 */
lexer::LexerRegistry &EngineController::lexer() {
    return lexer_;
}

/**
 * @brief Implements the `lexer` operation.
 *
 * @return Value produced by the operation.
 */
const lexer::LexerRegistry &EngineController::lexer() const {
    return lexer_;
}

/**
 * @brief Implements the `nodes` operation.
 *
 * @return Value produced by the operation.
 */
ast::NodeRegistry &EngineController::nodes() {
    return nodes_;
}

/**
 * @brief Implements the `nodes` operation.
 *
 * @return Value produced by the operation.
 */
const ast::NodeRegistry &EngineController::nodes() const {
    return nodes_;
}

/**
 * @brief Parses input through `parser`.
 *
 * @return Value produced by the operation.
 */
parser::ParserRegistry &EngineController::parser() {
    return parser_;
}

/**
 * @brief Parses input through `parser`.
 *
 * @return Value produced by the operation.
 */
const parser::ParserRegistry &EngineController::parser() const {
    return parser_;
}

/**
 * @brief Implements the `runtime` operation.
 *
 * @return Value produced by the operation.
 */
runtime::RuntimeRegistry &EngineController::runtime() {
    return runtime_;
}

/**
 * @brief Implements the `runtime` operation.
 *
 * @return Value produced by the operation.
 */
const runtime::RuntimeRegistry &EngineController::runtime() const {
    return runtime_;
}

/**
 * @brief Implements the `sources` operation.
 *
 * @return Value produced by the operation.
 */
source::SourceController &EngineController::sources() {
    return sources_;
}

/**
 * @brief Implements the `sources` operation.
 *
 * @return Value produced by the operation.
 */
const source::SourceController &EngineController::sources() const {
    return sources_;
}

/**
 * @brief Implements the `preprocessor` operation.
 *
 * @return Value produced by the operation.
 */
source::PreprocessorController &EngineController::preprocessor() {
    return preprocessor_;
}

/**
 * @brief Implements the `preprocessor` operation.
 *
 * @return Value produced by the operation.
 */
const source::PreprocessorController &EngineController::preprocessor() const {
    return preprocessor_;
}

/**
 * @brief Lowers input through `lowering`.
 *
 * @return Value produced by the operation.
 */
ir::LoweringRegistry &EngineController::lowering() {
    return lowering_;
}

/**
 * @brief Lowers input through `lowering`.
 *
 * @return Value produced by the operation.
 */
const ir::LoweringRegistry &EngineController::lowering() const {
    return lowering_;
}

/**
 * @brief Validates data through `validateFeatureInstall`.
 *
 * @param feature Value supplied for `feature`.
 */
void EngineController::validateFeatureInstall(const EngineFeature &feature) const {
    if (hasFeature(feature.name())) {
        throw std::runtime_error(
            "EngineController::validateFeatureInstall: duplicate feature '" + feature.name() + "'");
    }

    for (const std::string &conflict : feature.conflicts()) {
        if (hasFeature(conflict)) {
            throw std::runtime_error(
                "EngineController::validateFeatureInstall: feature '" + feature.name()
                + "' conflicts with installed feature '" + conflict + "'");
        }
    }

    for (const InstalledFeature &installed : features_) {
        const auto iter{std::find(installed.conflicts.begin(), installed.conflicts.end(), feature.name())};

        if (iter != installed.conflicts.end()) {
            throw std::runtime_error(
                "EngineController::validateFeatureInstall: installed feature '" + installed.name
                + "' conflicts with feature '" + feature.name() + "'");
        }
    }

    for (const std::string &requiredCapability : feature.requiredCapabilities()) {
        if (!hasCapability(requiredCapability)) {
            throw std::runtime_error(
                "EngineController::validateFeatureInstall: feature '" + feature.name()
                + "' requires missing capability '" + requiredCapability + "'");
        }
    }
}

/**
 * @brief Implements the `rememberFeature` operation.
 *
 * @param feature Value supplied for `feature`.
 */
void EngineController::rememberFeature(const EngineFeature &feature) {
    features_.push_back({
        feature.name(),
        feature.version(),
        feature.capabilities(),
        feature.conflicts()
    });

    for (const std::string &capability : feature.capabilities()) {
        registerCapability(capability);
    }
}

/**
 * @brief Returns the value required by `requireStartDomain`.
 *
 * @param owner Value supplied for `owner`.
 */
void EngineController::requireStartDomain(const std::string &owner) const {
    if (startDomain_.empty()) {
        const std::string message{owner + ": start domain is not configured"};
        diagnostics_.error(message);
        throw std::runtime_error(message);
    }
}



} // namespace novac::controllers
