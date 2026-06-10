#include "../../../include/novac/engine/EngineController.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace novac::controllers {

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

EngineFeature &EngineFeature::version(std::string value) {
    if (value.empty()) {
        throw std::runtime_error("EngineFeature::version: version cannot be empty");
    }

    version_ = std::move(value);

    return *this;
}

EngineFeature &EngineFeature::description(std::string value) {
    description_ = std::move(value);

    return *this;
}

EngineFeature &EngineFeature::provides(std::string capability) {
    if (capability.empty()) {
        throw std::runtime_error("EngineFeature::provides: capability cannot be empty");
    }

    capabilities_.push_back(std::move(capability));

    return *this;
}

EngineFeature &EngineFeature::requires(std::string capability) {
    if (capability.empty()) {
        throw std::runtime_error("EngineFeature::requires: capability cannot be empty");
    }

    requiredCapabilities_.push_back(std::move(capability));

    return *this;
}

EngineFeature &EngineFeature::conflictsWith(std::string featureName) {
    if (featureName.empty()) {
        throw std::runtime_error("EngineFeature::conflictsWith: feature name cannot be empty");
    }

    conflicts_.push_back(std::move(featureName));

    return *this;
}

EngineFeature &EngineFeature::onInstall(Installer installer) {
    if (!installer) {
        throw std::runtime_error("EngineFeature::onInstall: installer cannot be empty");
    }

    installers_.push_back(std::move(installer));

    return *this;
}

const std::string &EngineFeature::name() const {
    return name_;
}

const std::string &EngineFeature::version() const {
    return version_;
}

const std::string &EngineFeature::description() const {
    return description_;
}

const std::vector<std::string> &EngineFeature::capabilities() const {
    return capabilities_;
}

const std::vector<std::string> &EngineFeature::requiredCapabilities() const {
    return requiredCapabilities_;
}

const std::vector<std::string> &EngineFeature::conflicts() const {
    return conflicts_;
}

void EngineFeature::install(EngineController &engine) const {
    for (const Installer &installer : installers_) {
        installer(engine);
    }
}

EngineController::EngineController(EngineControllerOptions options)
    : lexer_{options.duplicatePolicy},
      nodes_{options.duplicatePolicy},
      parser_{options.duplicatePolicy},
      runtime_{options.duplicatePolicy},
      lowering_{options.duplicatePolicy},
      diagnostics_{},
      startDomain_{std::move(options.startDomain)},
      features_{} {
}

registry::RegisterStatus EngineController::keyword(std::string keyword) {
    return lexer_.keyword(std::move(keyword));
}

registry::RegisterStatus EngineController::symbol(std::string symbol) {
    return lexer_.symbol(std::move(symbol));
}

registry::RegisterStatus EngineController::node(ast::NodeSchema schema) {
    return nodes_.registerNode(std::move(schema));
}

registry::RegisterStatus EngineController::parseRule(std::string domain, std::string key, parser::ParseFn fn) {
    return parser_.rule(std::move(domain), std::move(key), std::move(fn));
}

registry::RegisterStatus EngineController::parseRule(const ids::ParseDomain &domain, std::string key, parser::ParseFn fn) {
    return parser_.rule(domain, std::move(key), std::move(fn));
}

registry::RegisterStatus EngineController::fallback(std::string domain, parser::ParseFn fn) {
    return parser_.fallback(std::move(domain), std::move(fn));
}

registry::RegisterStatus EngineController::fallback(const ids::ParseDomain &domain, parser::ParseFn fn) {
    return parser_.fallback(domain, std::move(fn));
}

registry::RegisterStatus EngineController::prefix(std::string domain, std::string key, parser::PrefixFn fn) {
    return parser_.prefix(std::move(domain), std::move(key), std::move(fn));
}

registry::RegisterStatus EngineController::prefix(const ids::ParseDomain &domain, std::string key, parser::PrefixFn fn) {
    return parser_.prefix(domain, std::move(key), std::move(fn));
}

registry::RegisterStatus EngineController::infix(std::string domain, std::string op, int precedence, parser::InfixFn fn) {
    return parser_.infix(std::move(domain), std::move(op), precedence, std::move(fn));
}

registry::RegisterStatus EngineController::infix(const ids::ParseDomain &domain, std::string op, int precedence, parser::InfixFn fn) {
    return parser_.infix(domain, std::move(op), precedence, std::move(fn));
}

registry::RegisterStatus EngineController::postfix(std::string domain, std::string op, int precedence, parser::PostfixFn fn) {
    return parser_.postfix(std::move(domain), std::move(op), precedence, std::move(fn));
}

registry::RegisterStatus EngineController::postfix(const ids::ParseDomain &domain, std::string op, int precedence, parser::PostfixFn fn) {
    return parser_.postfix(domain, std::move(op), precedence, std::move(fn));
}

registry::RegisterStatus EngineController::expression(std::string kind, runtime::ExprHandler handler) {
    return runtime_.expression(std::move(kind), std::move(handler));
}

registry::RegisterStatus EngineController::expression(const ids::NodeKind &kind, runtime::ExprHandler handler) {
    return runtime_.expression(kind, std::move(handler));
}

registry::RegisterStatus EngineController::statement(std::string kind, runtime::StmtHandler handler) {
    return runtime_.statement(std::move(kind), std::move(handler));
}

registry::RegisterStatus EngineController::statement(const ids::NodeKind &kind, runtime::StmtHandler handler) {
    return runtime_.statement(kind, std::move(handler));
}

registry::RegisterStatus EngineController::declaration(std::string kind, runtime::DeclHandler handler) {
    return runtime_.declaration(std::move(kind), std::move(handler));
}

registry::RegisterStatus EngineController::declaration(const ids::NodeKind &kind, runtime::DeclHandler handler) {
    return runtime_.declaration(kind, std::move(handler));
}

registry::RegisterStatus EngineController::binaryOperator(std::string op, runtime::BinaryHandler handler) {
    return runtime_.binaryOperator(std::move(op), std::move(handler));
}

registry::RegisterStatus EngineController::binaryOperator(const ids::Operation &op, runtime::BinaryHandler handler) {
    return runtime_.binaryOperator(op, std::move(handler));
}

registry::RegisterStatus EngineController::hir(std::string nodeKind, ir::LoweringRegistry::HIRLowerer lowerer) {
    return lowering_.hir(std::move(nodeKind), std::move(lowerer));
}

registry::RegisterStatus EngineController::hir(const ids::NodeKind &nodeKind, ir::LoweringRegistry::HIRLowerer lowerer) {
    return lowering_.hir(nodeKind, std::move(lowerer));
}

registry::RegisterStatus EngineController::mir(std::string instructionKind, ir::LoweringRegistry::MIRLowerer lowerer) {
    return lowering_.mir(std::move(instructionKind), std::move(lowerer));
}

registry::RegisterStatus EngineController::mir(const ids::Operation &instructionKind, ir::LoweringRegistry::MIRLowerer lowerer) {
    return lowering_.mir(instructionKind, std::move(lowerer));
}

ast::NodePtr EngineController::makeNode(std::string kind) const {
    return ast::Node::make(std::move(kind));
}

ast::NodePtr EngineController::makeNode(const ids::NodeKind &kind) const {
    return ast::Node::make(kind);
}

std::vector<token::Token> EngineController::tokenize(const std::string &source) const {
    lexer::Lexer lexer{lexer_};

    return lexer.tokenize(source);
}

ast::NodePtr EngineController::parse(const std::string &source) const {
    requireStartDomain("EngineController::parse");

    return parse(source, startDomain_);
}

ast::NodePtr EngineController::parse(const std::string &source, std::string startDomain) const {
    return parseTokens(tokenize(source), std::move(startDomain));
}

ast::NodePtr EngineController::parseTokens(std::vector<token::Token> tokens) const {
    requireStartDomain("EngineController::parseTokens");

    return parseTokens(std::move(tokens), startDomain_);
}

ast::NodePtr EngineController::parseTokens(std::vector<token::Token> tokens, std::string startDomain) const {
    if (startDomain.empty()) {
        throw std::runtime_error("EngineController::parseTokens: start domain cannot be empty");
    }

    parser::Parser parser{parser_, std::move(startDomain)};

    return parser.parse(std::move(tokens));
}

void EngineController::validate(const ast::Node &node) const {
    nodes_.validate(node);
}

runtime::Value EngineController::eval(const ast::Node &node) const {
    runtime::Runtime runtime{runtime_};

    return runtime.eval(node);
}

void EngineController::exec(const ast::Node &node) const {
    runtime::Runtime runtime{runtime_};

    runtime.exec(node);
}

ir::HIRModule EngineController::lowerToHIR(const ast::Node &node) const {
    ir::ASTLoweringPass pass{lowering_};

    return pass.lower(node);
}

ir::MIRModule EngineController::lowerToMIR(const ir::HIRModule &hir) const {
    ir::HIRLoweringPass pass{lowering_};

    return pass.lower(hir);
}

ir::MIRModule EngineController::lowerToMIR(const ast::Node &node) const {
    ir::HIRModule hir{lowerToHIR(node)};

    return lowerToMIR(hir);
}

void EngineController::install(const EngineFeature &feature) {
    validateFeatureInstall(feature);
    feature.install(*this);
    rememberFeature(feature);
}

bool EngineController::hasFeature(const std::string &name) const {
    return std::any_of(
        features_.begin(),
        features_.end(),
        [&](const InstalledFeature &feature) {
            return feature.name == name;
        });
}

bool EngineController::hasCapability(const std::string &capability) const {
    for (const InstalledFeature &feature : features_) {
        const auto iter{std::find(feature.capabilities.begin(), feature.capabilities.end(), capability)};

        if (iter != feature.capabilities.end()) {
            return true;
        }
    }

    return false;
}

void EngineController::setStartDomain(std::string startDomain) {
    if (startDomain.empty()) {
        throw std::runtime_error("EngineController::setStartDomain: start domain cannot be empty");
    }

    startDomain_ = std::move(startDomain);
}

const std::string &EngineController::startDomain() const {
    return startDomain_;
}

diagnostics::DiagnosticEngine &EngineController::diagnostics() {
    return diagnostics_;
}

const diagnostics::DiagnosticEngine &EngineController::diagnostics() const {
    return diagnostics_;
}

lexer::LexerRegistry &EngineController::lexer() {
    return lexer_;
}

const lexer::LexerRegistry &EngineController::lexer() const {
    return lexer_;
}

ast::NodeRegistry &EngineController::nodes() {
    return nodes_;
}

const ast::NodeRegistry &EngineController::nodes() const {
    return nodes_;
}

parser::ParserRegistry &EngineController::parser() {
    return parser_;
}

const parser::ParserRegistry &EngineController::parser() const {
    return parser_;
}

runtime::RuntimeRegistry &EngineController::runtime() {
    return runtime_;
}

const runtime::RuntimeRegistry &EngineController::runtime() const {
    return runtime_;
}

ir::LoweringRegistry &EngineController::lowering() {
    return lowering_;
}

const ir::LoweringRegistry &EngineController::lowering() const {
    return lowering_;
}

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

void EngineController::rememberFeature(const EngineFeature &feature) {
    features_.push_back({
        feature.name(),
        feature.version(),
        feature.capabilities(),
        feature.conflicts()
    });
}

void EngineController::requireStartDomain(const std::string &owner) const {
    if (startDomain_.empty()) {
        throw std::runtime_error(owner + ": start domain is not configured");
    }
}

} // namespace novac::controllers
