#pragma once

#include "execution/Runtime.hpp"
#include "foundation/Diagnostic.hpp"
#include "foundation/Ids.hpp"
#include "foundation/registry/Registry.hpp"
#include "syntax/Lexer.hpp"
#include "syntax/Node.hpp"
#include "syntax/Parser.hpp"
#include "syntax/Token.hpp"
#include "transformation/IR.hpp"

#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

namespace novac::controllers {

class EngineController;

/**
 * @brief Describes an installable extension for an engine controller.
 *
 * EngineFeature groups the configuration needed to add one language feature to
 * an EngineController.
 *
 * A feature can declare capabilities it provides, capabilities it requires,
 * incompatible features and one or more installer callbacks.
 *
 * Installer callbacks are executed in registration order when the feature is
 * installed.
 */
class EngineFeature final {
public:
    /**
     * @brief Function used to install a feature into an engine.
     */
    using Installer = std::function<void(EngineController &)>;

    /**
     * @brief Creates a feature descriptor.
     *
     * New features start with version "0.1.0" and no capabilities,
     * dependencies, conflicts or installers.
     *
     * @param name Unique feature name.
     *
     * @throws std::runtime_error If name is empty.
     */
    explicit EngineFeature(std::string name);

    /**
     * @brief Sets the feature version.
     *
     * @param value Version string.
     * @return Reference to this feature for chaining.
     *
     * @throws std::runtime_error If value is empty.
     */
    EngineFeature &version(std::string value);

    /**
     * @brief Sets the feature description.
     *
     * @param value Description text.
     * @return Reference to this feature for chaining.
     */
    EngineFeature &description(std::string value);

    /**
     * @brief Declares a capability provided by this feature.
     *
     * Provided capabilities become available to features installed later.
     *
     * @param capability Capability name.
     * @return Reference to this feature for chaining.
     *
     * @throws std::runtime_error If capability is empty.
     */
    EngineFeature &provides(std::string capability);

    /**
     * @brief Declares a capability required before this feature can be installed.
     *
     * Installation fails if no previously installed feature provides the
     * requested capability.
     *
     * @param capability Required capability name.
     * @return Reference to this feature for chaining.
     *
     * @throws std::runtime_error If capability is empty.
     */
    EngineFeature &requiresCapability(std::string capability);

    /**
     * @brief Declares a capability dependency.
     *
     * This is an alias for requiresCapability().
     *
     * @param capability Required capability name.
     * @return Reference to this feature for chaining.
     *
     * @throws std::runtime_error If capability is empty.
     */
    EngineFeature &dependsOn(std::string capability);

    /**
     * @brief Declares a conflicting feature.
     *
     * The feature cannot be installed while a feature with this name is
     * already installed.
     *
     * @param featureName Name of an incompatible feature.
     * @return Reference to this feature for chaining.
     *
     * @throws std::runtime_error If featureName is empty.
     */
    EngineFeature &conflictsWith(std::string featureName);

    /**
     * @brief Adds an installer callback.
     *
     * Installers usually register lexer, parser, AST, runtime or lowering
     * behavior through EngineController.
     *
     * Multiple installers are allowed and are executed in registration order.
     *
     * @param installer Callback invoked during feature installation.
     * @return Reference to this feature for chaining.
     *
     * @throws std::runtime_error If installer is empty.
     */
    EngineFeature &onInstall(Installer installer);

    /**
     * @brief Returns the feature name.
     *
     * @return Feature name.
     */
    const std::string &name() const;

    /**
     * @brief Returns the feature version.
     *
     * @return Version string.
     */
    const std::string &version() const;

    /**
     * @brief Returns the feature description.
     *
     * @return Description text.
     */
    const std::string &description() const;

    /**
     * @brief Returns capabilities provided by this feature.
     *
     * @return Provided capability names.
     */
    const std::vector<std::string> &capabilities() const;

    /**
     * @brief Returns capabilities required by this feature.
     *
     * @return Required capability names.
     */
    const std::vector<std::string> &requiredCapabilities() const;

    /**
     * @brief Returns feature names that conflict with this feature.
     *
     * @return Conflicting feature names.
     */
    const std::vector<std::string> &conflicts() const;

    /**
     * @brief Runs this feature's installer callbacks on an engine.
     *
     * Installers are invoked sequentially in the same order they were added
     * with onInstall().
     *
     * @param engine Engine controller to mutate.
     */
    void install(EngineController &engine) const;

private:
    std::string name_;
    std::string version_;
    std::string description_;
    std::vector<std::string> capabilities_;
    std::vector<std::string> requiredCapabilities_;
    std::vector<std::string> conflicts_;
    std::vector<Installer> installers_;
};

/**
 * @brief Construction options for EngineController.
 *
 * The duplicate policy is forwarded to the internal registries while
 * startDomain configures the default parser entry point.
 */
struct EngineControllerOptions final {
    /** Duplicate handling policy shared by engine registries. */
    registry::DuplicatePolicy duplicatePolicy{registry::DuplicatePolicy::Error};

    /** Default parser domain used by parse() and parseTokens(). */
    std::string startDomain{};
};

/**
 * @brief Coordinates the configurable language engine.
 *
 * EngineController is the high-level façade used to construct and operate a
 * NovaC language.
 *
 * It owns the lexer, AST, parser, runtime and lowering registries together with
 * the diagnostic engine and installed feature metadata.
 *
 * Most registration functions simply forward to the appropriate subsystem,
 * allowing language features to configure the complete pipeline through one
 * common interface.
 */
class EngineController final {
public:
    /**
     * @brief Creates an engine controller.
     *
     * The configured duplicate policy is applied to the AST, parser, runtime
     * and lowering registries created by the controller.
     *
     * @param options Initial controller options.
     */
    explicit EngineController(EngineControllerOptions options = {});

    /**
     * @brief Registers a lexer keyword.
     *
     * The registration is forwarded to the internal LexerRegistry.
     *
     * @param keyword Keyword text.
     * @return Registration result.
     *
     * @throws std::runtime_error If keyword is empty or duplicate registration is rejected.
     */
    registry::RegisterStatus keyword(std::string keyword);

    /**
     * @brief Registers a lexer symbol.
     *
     * The registration is forwarded to the internal LexerRegistry.
     *
     * @param symbol Symbol text.
     * @return Registration result.
     *
     * @throws std::runtime_error If symbol is empty or duplicate registration is rejected.
     */
    registry::RegisterStatus symbol(std::string symbol);

    /**
     * @brief Registers an AST node schema.
     *
     * The schema is stored in the internal NodeRegistry and later used during
     * AST validation.
     *
     * @param schema Node schema to register.
     * @return Registration result.
     *
     * @throws std::runtime_error If the schema is invalid or duplicate registration is rejected.
     */
    registry::RegisterStatus node(ast::NodeSchema schema);

    /**
     * @brief Registers a parser rule.
     *
     * @param domain Parse domain name.
     * @param key Token key or token text matched by the rule.
     * @param fn Parse function.
     * @return Registration result.
     *
     * @throws std::runtime_error If arguments are invalid or duplicate registration is rejected.
     */
    registry::RegisterStatus parseRule(std::string domain, std::string key, parser::ParseFn fn);

    /**
     * @brief Registers a parser rule using a typed domain identifier.
     *
     * @param domain Parse domain identifier.
     * @param key Token key or token text matched by the rule.
     * @param fn Parse function.
     * @return Registration result.
     *
     * @throws std::runtime_error If arguments are invalid or duplicate registration is rejected.
     */
    registry::RegisterStatus parseRule(const ids::ParseDomain &domain, std::string key, parser::ParseFn fn);

    /**
     * @brief Registers a fallback parser rule.
     *
     * Fallback rules are tried when the parser has no direct or Pratt rule for
     * the current token.
     *
     * @param domain Parse domain name.
     * @param fn Fallback parse function.
     * @return Registration result.
     *
     * @throws std::runtime_error If domain or fn is empty.
     */
    registry::RegisterStatus fallback(std::string domain, parser::ParseFn fn);

    /**
     * @brief Registers a fallback parser rule using a typed domain identifier.
     *
     * @param domain Parse domain identifier.
     * @param fn Fallback parse function.
     * @return Registration result.
     *
     * @throws std::runtime_error If fn is empty.
     */
    registry::RegisterStatus fallback(const ids::ParseDomain &domain, parser::ParseFn fn);

    /**
     * @brief Registers a Pratt prefix rule.
     *
     * Prefix rules are used to begin expression parsing before infix and
     * postfix operators are processed.
     *
     * @param domain Parse domain name.
     * @param key Token key or token text matched by the prefix rule.
     * @param fn Prefix parse function.
     * @return Registration result.
     *
     * @throws std::runtime_error If arguments are invalid or duplicate registration is rejected.
     */
    registry::RegisterStatus prefix(std::string domain, std::string key, parser::PrefixFn fn);

    /**
     * @brief Registers a Pratt prefix rule using a typed domain identifier.
     *
     * @param domain Parse domain identifier.
     * @param key Token key or token text matched by the prefix rule.
     * @param fn Prefix parse function.
     * @return Registration result.
     *
     * @throws std::runtime_error If arguments are invalid or duplicate registration is rejected.
     */
    registry::RegisterStatus prefix(const ids::ParseDomain &domain, std::string key, parser::PrefixFn fn);

    /**
     * @brief Registers a transactional Pratt prefix fallback.
     *
     * The callback may consume tokens speculatively. Returning nullptr restores
     * the parser position and allows the next prefix candidate to run.
     */
    registry::RegisterStatus prefixFallback(std::string domain, std::string key, parser::PrefixFn fn);

    /** @brief Registers a transactional Pratt prefix fallback using a typed domain. */
    registry::RegisterStatus prefixFallback(const ids::ParseDomain &domain, std::string key, parser::PrefixFn fn);

    /**
     * @brief Registers a left-associative Pratt infix rule.
     *
     * This overload uses parser::Associativity::Left.
     *
     * @param domain Parse domain name.
     * @param op Operator token text.
     * @param precedence Operator precedence.
     * @param fn Infix parse function.
     * @return Registration result.
     *
     * @throws std::runtime_error If arguments are invalid or duplicate registration is rejected.
     */
    registry::RegisterStatus infix(std::string domain, std::string op, int precedence, parser::InfixFn fn);

    /**
     * @brief Registers a Pratt infix rule.
     *
     * @param domain Parse domain name.
     * @param op Operator token text.
     * @param precedence Operator precedence.
     * @param associativity Operator associativity.
     * @param fn Infix parse function.
     * @return Registration result.
     *
     * @throws std::runtime_error If arguments are invalid or duplicate registration is rejected.
     */
    registry::RegisterStatus infix(std::string domain, std::string op, int precedence, parser::Associativity associativity, parser::InfixFn fn);

    /**
     * @brief Registers a left-associative Pratt infix rule using a typed domain.
     *
     * @param domain Parse domain identifier.
     * @param op Operator token text.
     * @param precedence Operator precedence.
     * @param fn Infix parse function.
     * @return Registration result.
     *
     * @throws std::runtime_error If arguments are invalid or duplicate registration is rejected.
     */
    registry::RegisterStatus infix(const ids::ParseDomain &domain, std::string op, int precedence, parser::InfixFn fn);

    /**
     * @brief Registers a Pratt infix rule using a typed domain.
     *
     * @param domain Parse domain identifier.
     * @param op Operator token text.
     * @param precedence Operator precedence.
     * @param associativity Operator associativity.
     * @param fn Infix parse function.
     * @return Registration result.
     *
     * @throws std::runtime_error If arguments are invalid or duplicate registration is rejected.
     */
    registry::RegisterStatus infix(const ids::ParseDomain &domain, std::string op, int precedence, parser::Associativity associativity, parser::InfixFn fn);

    /**
     * @brief Registers a Pratt postfix rule.
     *
     * @param domain Parse domain name.
     * @param op Operator token text.
     * @param precedence Operator precedence.
     * @param fn Postfix parse function.
     * @return Registration result.
     *
     * @throws std::runtime_error If arguments are invalid or duplicate registration is rejected.
     */
    registry::RegisterStatus postfix(std::string domain, std::string op, int precedence, parser::PostfixFn fn);

    /**
     * @brief Registers a Pratt postfix rule using a typed domain identifier.
     *
     * @param domain Parse domain identifier.
     * @param op Operator token text.
     * @param precedence Operator precedence.
     * @param fn Postfix parse function.
     * @return Registration result.
     *
     * @throws std::runtime_error If arguments are invalid or duplicate registration is rejected.
     */
    registry::RegisterStatus postfix(const ids::ParseDomain &domain, std::string op, int precedence, parser::PostfixFn fn);

    /**
     * @brief Registers a runtime expression handler.
     *
     * The handler will be selected by AST node kind during evaluation.
     *
     * @param kind AST node kind handled as an expression.
     * @param handler Evaluation handler.
     * @return Registration result.
     *
     * @throws std::runtime_error If arguments are invalid or duplicate registration is rejected.
     */
    registry::RegisterStatus expression(std::string kind, runtime::ExprHandler handler);

    /**
     * @brief Registers a runtime expression handler using a typed node kind.
     *
     * @param kind AST node kind handled as an expression.
     * @param handler Evaluation handler.
     * @return Registration result.
     *
     * @throws std::runtime_error If handler is empty or duplicate registration is rejected.
     */
    registry::RegisterStatus expression(const ids::NodeKind &kind, runtime::ExprHandler handler);

    /**
     * @brief Registers a runtime statement handler.
     *
     * @param kind AST node kind handled as a statement.
     * @param handler Execution handler.
     * @return Registration result.
     *
     * @throws std::runtime_error If arguments are invalid or duplicate registration is rejected.
     */
    registry::RegisterStatus statement(std::string kind, runtime::StmtHandler handler);

    /**
     * @brief Registers a runtime statement handler using a typed node kind.
     *
     * @param kind AST node kind handled as a statement.
     * @param handler Execution handler.
     * @return Registration result.
     *
     * @throws std::runtime_error If handler is empty or duplicate registration is rejected.
     */
    registry::RegisterStatus statement(const ids::NodeKind &kind, runtime::StmtHandler handler);

    /**
     * @brief Registers a runtime declaration handler.
     *
     * @param kind AST node kind handled as a declaration.
     * @param handler Declaration handler.
     * @return Registration result.
     *
     * @throws std::runtime_error If arguments are invalid or duplicate registration is rejected.
     */
    registry::RegisterStatus declaration(std::string kind, runtime::DeclHandler handler);

    /**
     * @brief Registers a runtime declaration handler using a typed node kind.
     *
     * @param kind AST node kind handled as a declaration.
     * @param handler Declaration handler.
     * @return Registration result.
     *
     * @throws std::runtime_error If handler is empty or duplicate registration is rejected.
     */
    registry::RegisterStatus declaration(const ids::NodeKind &kind, runtime::DeclHandler handler);

    /**
     * @brief Registers a runtime binary operator handler.
     *
     * Registering the first binary operator also installs the runtime binary
     * dispatcher if it has not already been installed.
     *
     * @param op Operator text.
     * @param handler Binary operator handler.
     * @return Registration result.
     *
     * @throws std::runtime_error If arguments are invalid or duplicate registration is rejected.
     */
    registry::RegisterStatus binaryOperator(std::string op, runtime::BinaryHandler handler);

    /**
     * @brief Registers a runtime binary operator handler using a typed operation.
     *
     * @param op Operator identifier.
     * @param handler Binary operator handler.
     * @return Registration result.
     *
     * @throws std::runtime_error If handler is empty or duplicate registration is rejected.
     */
    registry::RegisterStatus binaryOperator(const ids::Operation &op, runtime::BinaryHandler handler);

    /**
     * @brief Sets the AST node kind used by the runtime binary dispatcher.
     *
     * This must be configured before the first binary operator causes the
     * dispatcher to be installed.
     *
     * @param kind Binary expression node kind.
     *
     * @throws std::runtime_error If kind is empty or the binary dispatcher has already been installed.
     */
    void setBinaryNodeKind(std::string kind);

    /**
     * @brief Registers an AST-to-HIR lowerer.
     *
     * During AST lowering, the node kind is used to select this function.
     *
     * @param nodeKind AST node kind.
     * @param lowerer Lowering function.
     * @return Registration result.
     *
     * @throws std::runtime_error If arguments are invalid or duplicate registration is rejected.
     */
    registry::RegisterStatus hir(std::string nodeKind, ir::LoweringRegistry::HIRLowerer lowerer);

    /**
     * @brief Registers an AST-to-HIR lowerer using a typed node kind.
     *
     * @param nodeKind AST node kind.
     * @param lowerer Lowering function.
     * @return Registration result.
     *
     * @throws std::runtime_error If lowerer is empty or duplicate registration is rejected.
     */
    registry::RegisterStatus hir(const ids::NodeKind &nodeKind, ir::LoweringRegistry::HIRLowerer lowerer);

    /**
     * @brief Registers a HIR-to-MIR lowerer.
     *
     * The HIR instruction operation is used to select this function while
     * lowering to MIR.
     *
     * @param instructionKind HIR instruction operation.
     * @param lowerer Lowering function.
     * @return Registration result.
     *
     * @throws std::runtime_error If arguments are invalid or duplicate registration is rejected.
     */
    registry::RegisterStatus mir(std::string instructionKind, ir::LoweringRegistry::MIRLowerer lowerer);

    /**
     * @brief Registers a HIR-to-MIR lowerer using a typed operation.
     *
     * @param instructionKind HIR instruction operation.
     * @param lowerer Lowering function.
     * @return Registration result.
     *
     * @throws std::runtime_error If lowerer is empty or duplicate registration is rejected.
     */
    registry::RegisterStatus mir(const ids::Operation &instructionKind, ir::LoweringRegistry::MIRLowerer lowerer);

    /**
     * @brief Creates an AST node.
     *
     * This is a convenience wrapper around ast::Node::make().
     *
     * @param kind Node kind.
     * @return Shared ownership of the created node.
     *
     * @throws std::runtime_error If kind is empty.
     */
    ast::NodePtr makeNode(std::string kind) const;

    /**
     * @brief Creates an AST node using a typed node kind.
     *
     * @param kind Node kind.
     * @return Shared ownership of the created node.
     */
    ast::NodePtr makeNode(const ids::NodeKind &kind) const;

    /**
     * @brief Tokenizes source text using the configured lexer registry.
     *
     * A temporary Lexer is created from the controller's LexerRegistry.
     *
     * @param source Source text.
     * @return Token stream terminated by an end token.
     */
    std::vector<token::Token> tokenize(const std::string &source) const;

    /**
     * @brief Parses source text using the configured start domain.
     *
     * The source is first tokenized, then parsed using startDomain().
     *
     * @param source Source text.
     * @return Parsed AST root.
     *
     * @throws std::runtime_error If no start domain is configured or parsing fails.
     */
    ast::NodePtr parse(const std::string &source) const;

    /**
     * @brief Parses source text using an explicit start domain.
     *
     * The source is tokenized and the resulting token stream is forwarded to
     * parseTokens().
     *
     * @param source Source text.
     * @param startDomain Parse start domain.
     * @return Parsed AST root.
     *
     * @throws std::runtime_error If startDomain is empty or parsing fails.
     */
    ast::NodePtr parse(const std::string &source, std::string startDomain) const;

    /**
     * @brief Parses tokens using the configured start domain.
     *
     * @param tokens Token stream.
     * @return Parsed AST root.
     *
     * @throws std::runtime_error If no start domain is configured or parsing fails.
     */
    ast::NodePtr parseTokens(std::vector<token::Token> tokens) const;

    /**
     * @brief Parses tokens using an explicit start domain.
     *
     * A temporary Parser is created from the controller's ParserRegistry and
     * the requested start domain.
     *
     * @param tokens Token stream.
     * @param startDomain Parse start domain.
     * @return Parsed AST root.
     *
     * @throws std::runtime_error If startDomain is empty or parsing fails.
     */
    ast::NodePtr parseTokens(std::vector<token::Token> tokens, std::string startDomain) const;

    /**
     * @brief Validates an AST node using the configured node registry.
     *
     * Validation is forwarded to NodeRegistry and recursively checks the AST
     * structure against registered schemas.
     *
     * @param node AST node to validate.
     *
     * @throws std::runtime_error If validation fails.
     */
    void validate(const ast::Node &node) const;

    /**
     * @brief Evaluates an AST expression.
     *
     * A temporary Runtime is created using the configured RuntimeRegistry.
     *
     * @param node AST node to evaluate.
     * @return Runtime value produced by evaluation.
     *
     * @throws std::runtime_error If no runtime handler is registered or evaluation fails.
     */
    runtime::Value eval(const ast::Node &node) const;

    /**
     * @brief Executes an AST statement.
     *
     * A temporary Runtime is created using the configured RuntimeRegistry.
     *
     * @param node AST node to execute.
     *
     * @throws std::runtime_error If no runtime handler is registered or execution fails.
     */
    void exec(const ast::Node &node) const;

    /**
     * @brief Lowers an AST node to HIR.
     *
     * A temporary ASTLoweringPass is created using the configured lowering
     * registry.
     *
     * @param node AST node to lower.
     * @return Produced HIR module.
     *
     * @throws std::runtime_error If required lowerers are missing.
     */
    ir::HIRModule lowerToHIR(const ast::Node &node) const;

    /**
     * @brief Lowers a HIR module to MIR.
     *
     * A temporary HIRLoweringPass is created using the configured lowering
     * registry.
     *
     * @param hir HIR module to lower.
     * @return Produced MIR module.
     *
     * @throws std::runtime_error If required lowerers are missing.
     */
    ir::MIRModule lowerToMIR(const ir::HIRModule &hir) const;

    /**
     * @brief Lowers an AST node directly to MIR.
     *
     * The AST is first lowered to HIR, then the produced HIR module is lowered
     * to MIR.
     *
     * @param node AST node to lower.
     * @return Produced MIR module.
     *
     * @throws std::runtime_error If required lowerers are missing.
     */
    ir::MIRModule lowerToMIR(const ast::Node &node) const;

    /**
     * @brief Installs a feature atomically.
     *
     * Installation first validates duplicate features, conflicts and required
     * capabilities.
     *
     * The feature is then installed into a copy of the current controller.
     * Only after every installer succeeds and the feature metadata is recorded
     * is the candidate moved back into this controller.
     *
     * This prevents partially applied feature installations from modifying the
     * original engine state.
     *
     * @param feature Feature to install.
     *
     * @throws std::runtime_error If the feature is duplicate, conflicts, lacks dependencies, or installation fails.
     */
    void install(const EngineFeature &feature);

    /**
     * @brief Creates a copy of the current engine state.
     *
     * The returned controller contains copies of the registries, diagnostics,
     * start domain and installed feature metadata.
     *
     * @return Snapshot of this controller.
     */
    EngineController snapshot() const;

    /**
     * @brief Replaces this controller with a previous snapshot.
     *
     * @param snapshot Snapshot to restore.
     */
    void restore(const EngineController &snapshot);

    /**
     * @brief Checks whether a feature is installed.
     *
     * Installed feature metadata is searched by feature name.
     *
     * @param name Feature name.
     * @return True if a feature with the name is installed.
     */
    bool hasFeature(const std::string &name) const;

    /**
     * @brief Checks whether an installed feature provides a capability.
     *
     * Every installed feature is searched until the requested capability is
     * found.
     *
     * @param capability Capability name.
     * @return True if the capability is available.
     */
    bool hasCapability(const std::string &capability) const;

    /**
     * @brief Publishes a capability on this engine.
     *
     * Asset controllers use this to expose capabilities from features that do
     * not use EngineFeature directly. Registering an already available
     * capability is idempotent.
     *
     * @param capability Capability name to publish.
     * @throws std::runtime_error If capability is empty.
     */
    void registerCapability(std::string capability);

    /**
     * @brief Sets the default parser start domain.
     *
     * This domain is used by parse(source) and parseTokens(tokens).
     *
     * @param startDomain Parse start domain.
     *
     * @throws std::runtime_error If startDomain is empty.
     */
    void setStartDomain(std::string startDomain);

    /**
     * @brief Returns the default parser start domain.
     *
     * @return Current start domain.
     */
    const std::string &startDomain() const;

    /**
     * @brief Returns the diagnostic engine.
     *
     * @return Mutable diagnostic engine.
     */
    diagnostics::DiagnosticEngine &diagnostics();

    /**
     * @brief Returns the diagnostic engine.
     *
     * @return Diagnostic engine.
     */
    const diagnostics::DiagnosticEngine &diagnostics() const;

    /**
     * @brief Returns the lexer registry.
     *
     * Direct access can be used for lexer configuration not exposed by the
     * EngineController convenience functions.
     *
     * @return Mutable lexer registry.
     */
    lexer::LexerRegistry &lexer();

    /**
     * @brief Returns the lexer registry.
     *
     * @return Lexer registry.
     */
    const lexer::LexerRegistry &lexer() const;

    /**
     * @brief Returns the AST node registry.
     *
     * @return Mutable node registry.
     */
    ast::NodeRegistry &nodes();

    /**
     * @brief Returns the AST node registry.
     *
     * @return Node registry.
     */
    const ast::NodeRegistry &nodes() const;

    /**
     * @brief Returns the parser registry.
     *
     * @return Mutable parser registry.
     */
    parser::ParserRegistry &parser();

    /**
     * @brief Returns the parser registry.
     *
     * @return Parser registry.
     */
    const parser::ParserRegistry &parser() const;

    /**
     * @brief Returns the runtime registry.
     *
     * @return Mutable runtime registry.
     */
    runtime::RuntimeRegistry &runtime();

    /**
     * @brief Returns the runtime registry.
     *
     * @return Runtime registry.
     */
    const runtime::RuntimeRegistry &runtime() const;

    /**
     * @brief Returns the lowering registry.
     *
     * @return Mutable lowering registry.
     */
    ir::LoweringRegistry &lowering();

    /**
     * @brief Returns the lowering registry.
     *
     * @return Lowering registry.
     */
    const ir::LoweringRegistry &lowering() const;

private:
    /**
     * @brief Stored metadata for an installed feature.
     *
     * Only information needed for capability and conflict checks is retained.
     */
    struct InstalledFeature final {
        std::string name{};
        std::string version{};
        std::vector<std::string> capabilities{};
        std::vector<std::string> conflicts{};
    };

    /**
     * @brief Validates whether a feature can be installed.
     *
     * The check rejects duplicate feature names, conflicts declared in either
     * direction and missing required capabilities.
     */
    void validateFeatureInstall(const EngineFeature &feature) const;

    /**
     * @brief Stores metadata for a successfully installed feature.
     */
    void rememberFeature(const EngineFeature &feature);

    /**
     * @brief Ensures that a default parser start domain is configured.
     *
     * @param owner Function name used in the error message.
     *
     * @throws std::runtime_error If no start domain is configured.
     */
    void requireStartDomain(const std::string &owner) const;

    lexer::LexerRegistry lexer_;
    ast::NodeRegistry nodes_;
    parser::ParserRegistry parser_;
    runtime::RuntimeRegistry runtime_;
    ir::LoweringRegistry lowering_;
    diagnostics::DiagnosticEngine diagnostics_;
    std::string startDomain_;
    std::vector<InstalledFeature> features_;
    std::unordered_set<std::string> capabilities_;
};

} // namespace novac::controllers