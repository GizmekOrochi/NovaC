#include "novac/engine/compilation/CompilationController.hpp"

#include <iterator>
#include <stdexcept>
#include <utility>

namespace novac::controllers {

namespace {

/**
 * @brief Returns the value required by `requireArtifactName`.
 *
 * @param value Value supplied for `value`.
 * @param field Value supplied for `field`.
 */
void requireArtifactName(const std::string &value, const char *field) {
    if (value.empty()) {
        throw std::runtime_error(std::string{"CompilationController: artifact name '"} + field + "' cannot be empty");
    }
}

/**
 * @brief Validates data through `validateArtifactNames`.
 *
 * @param names Value supplied for `names`.
 */
void validateArtifactNames(const CompilationArtifactNames &names) {
    requireArtifactName(names.source, "source");
    requireArtifactName(names.preprocessedSource, "preprocessedSource");
    requireArtifactName(names.tokens, "tokens");
    requireArtifactName(names.ast, "ast");
    requireArtifactName(names.hir, "hir");
    requireArtifactName(names.mir, "mir");
}

/**
 * @brief Validates data through `validateRunOptions`.
 *
 * @param options Value supplied for `options`.
 */
void validateRunOptions(const CompilationOptions &options) {
    if (options.produceMIR && !options.produceHIR) {
        throw std::runtime_error("CompilationController: produceMIR requires produceHIR");
    }
}

/**
 * @brief Returns the value required by `requireStep`.
 *
 * @param fn Value supplied for `fn`.
 * @param name Value supplied for `name`.
 * @return Value produced by the operation.
 */
template <typename Fn>
const Fn &requireStep(const Fn &fn, const char *name) {
    if (!fn) {
        throw std::runtime_error(std::string{"CompilationController: required step '"} + name + "' is not configured");
    }
    return fn;
}

} // namespace

/**
 * @brief Implements the `classicCompilationSteps` operation.
 *
 * @param engine Value supplied for `engine`.
 * @return Value produced by the operation.
 */
CompilationSteps classicCompilationSteps(EngineController &engine) {
    CompilationSteps steps{};

    steps.resolveSource = [&engine](const std::string &specifier, compilation::CompilationSession &) {
        if (specifier.empty()) {
            throw std::runtime_error("classicCompilationSteps: source specifier cannot be empty");
        }
        return engine.sources().resolve({specifier, {}});
    };

    steps.preprocess = [&engine](const source::Source &sourceValue, const source::PreprocessOptions &options, compilation::CompilationSession &session) {
        return engine.preprocessor().process(    sourceValue,     engine.sources(),     engine.lexer(),     session.diagnostics(),     options);
    };

    steps.tokenize = [&engine](const std::string &text, std::optional<diagnostics::SourceLocation> origin, compilation::CompilationSession &) {
        lexer::Lexer lexer{engine.lexer()};
        if (origin.has_value()) {
            return lexer.tokenize(text, std::move(*origin));
        }
        return lexer.tokenize(text);
    };

    steps.parse = [&engine](std::vector<token::Token> tokens, const std::string &startDomain, compilation::CompilationSession &) {
        parser::Parser parser{engine.parser(), startDomain};
        return parser.parse(tokens);
    };

    steps.validateAst = [&engine](const ast::Node &root, compilation::CompilationSession &) {
        engine.nodes().validate(root);
    };

    steps.lowerHIR = [&engine](const ast::Node &root, compilation::CompilationSession &) {
        ir::ASTLoweringPass lowerer{engine.lowering()};
        return lowerer.lower(root);
    };

    steps.lowerMIR = [&engine](const ir::HIRModule &hir, compilation::CompilationSession &) {
        ir::HIRLoweringPass lowerer{engine.lowering()};
        return lowerer.lower(hir);
    };

    steps.defaultStartDomain = [&engine] {
        return engine.startDomain();
    };

    return steps;
}

/**
 * @brief Constructs a `CompilationController` instance.
 *
 * @param engine Value supplied for `engine`.
 * @param options Value supplied for `options`.
 */
CompilationController::CompilationController(EngineController &engine, CompilationControllerOptions options)
    : engine_{&engine},
      options_{std::move(options)},
      steps_{classicCompilationSteps(engine)},
      passes_{options_.duplicatePolicy},
      backends_{options_.duplicatePolicy} {
    validateArtifactNames(options_.artifacts);
}

/**
 * @brief Constructs a `CompilationController` instance.
 *
 * @param steps Value supplied for `steps`.
 * @param options Value supplied for `options`.
 */
CompilationController::CompilationController(CompilationSteps steps, CompilationControllerOptions options)
    : options_{std::move(options)},
      steps_{std::move(steps)},
      passes_{options_.duplicatePolicy},
      backends_{options_.duplicatePolicy} {
    validateArtifactNames(options_.artifacts);
}

/**
 * @brief Implements the `pass` operation.
 *
 * @param descriptor Value supplied for `descriptor`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus CompilationController::pass(compilation::PassDescriptor descriptor) {
    return passes_.add(std::move(descriptor));
}

/**
 * @brief Implements the `backend` operation.
 *
 * @param id Value supplied for `id`.
 * @param implementation Value supplied for `implementation`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus CompilationController::backend(std::string id, std::shared_ptr<const novac::backend::Backend> implementation) {
    return backends_.add(std::move(id), std::move(implementation));
}

/**
 * @brief Implements the `passes` operation.
 *
 * @return Value produced by the operation.
 */
compilation::PassRegistry &CompilationController::passes() noexcept {
    return passes_;
}

/**
 * @brief Implements the `passes` operation.
 *
 * @return Value produced by the operation.
 */
const compilation::PassRegistry &CompilationController::passes() const noexcept {
    return passes_;
}

/**
 * @brief Implements the `backends` operation.
 *
 * @return Value produced by the operation.
 */
novac::backend::BackendRegistry &CompilationController::backends() noexcept {
    return backends_;
}

/**
 * @brief Implements the `backends` operation.
 *
 * @return Value produced by the operation.
 */
const novac::backend::BackendRegistry &CompilationController::backends() const noexcept {
    return backends_;
}

/**
 * @brief Implements the `steps` operation.
 *
 * @return Value produced by the operation.
 */
CompilationSteps &CompilationController::steps() noexcept {
    return steps_;
}

/**
 * @brief Implements the `steps` operation.
 *
 * @return Value produced by the operation.
 */
const CompilationSteps &CompilationController::steps() const noexcept {
    return steps_;
}

/**
 * @brief Checks the condition represented by `hasEngine`.
 *
 * @return Value produced by the operation.
 */
bool CompilationController::hasEngine() const noexcept {
    return engine_ != nullptr;
}

/**
 * @brief Returns the value exposed by `engine`.
 *
 * @return Value produced by the operation.
 */
EngineController &CompilationController::engine() {
    if (!engine_) {
        throw std::runtime_error("CompilationController::engine: controller is not bound to an EngineController");
    }
    return *engine_;
}

/**
 * @brief Returns the value exposed by `engine`.
 *
 * @return Value produced by the operation.
 */
const EngineController &CompilationController::engine() const {
    if (!engine_) {
        throw std::runtime_error("CompilationController::engine: controller is not bound to an EngineController");
    }
    return *engine_;
}

/**
 * @brief Implements the `options` operation.
 *
 * @return Value produced by the operation.
 */
const CompilationControllerOptions &CompilationController::options() const noexcept {
    return options_;
}

/**
 * @brief Starts the operation represented by `startDomainFor`.
 *
 * @param options Value supplied for `options`.
 * @return Value produced by the operation.
 */
std::string CompilationController::startDomainFor(const CompilationOptions &options) const {
    if (!options.startDomain.empty()) {
        return options.startDomain;
    }
    if (!options_.defaultStartDomain.empty()) {
        return options_.defaultStartDomain;
    }
    if (steps_.defaultStartDomain) {
        const std::string value{steps_.defaultStartDomain()};
        if (!value.empty()) {
            return value;
        }
    }
    throw std::runtime_error("CompilationController: no parser start domain configured");
}

/**
 * @brief Implements the `backendFor` operation.
 *
 * @param options Value supplied for `options`.
 * @return Value produced by the operation.
 */
std::string CompilationController::backendFor(const CompilationOptions &options) const {
    if (!options.backend.empty()) {
        return options.backend;
    }
    return options_.defaultBackend;
}

/**
 * @brief Implements the `runStage` operation.
 *
 * @param stage Value supplied for `stage`.
 * @param session Value supplied for `session`.
 */
void CompilationController::runStage(const std::string &stage, compilation::CompilationSession &session) const {
    if (!stage.empty()) {
        passes_.run(stage, session);
    }
}

/**
 * @brief Parses input through `parseTokensIntoSession`.
 *
 * @param tokens Value supplied for `tokens`.
 * @param options Value supplied for `options`.
 * @param session Value supplied for `session`.
 * @return Value produced by the operation.
 */
ast::NodePtr CompilationController::parseTokensIntoSession(std::vector<token::Token> tokens, const CompilationOptions &options, compilation::CompilationSession &session) const {
    session.artifacts().set<std::vector<token::Token>>(options_.artifacts.tokens, std::move(tokens));
    runStage(options_.stages.tokens, session);

    auto &currentTokens{session.artifacts().require<std::vector<token::Token>>(options_.artifacts.tokens)};
    return requireStep(steps_.parse, "parse")(currentTokens, startDomainFor(options), session);
}

/**
 * @brief Implements the `compileAstIntoSession` operation.
 *
 * @param root Value supplied for `root`.
 * @param options Value supplied for `options`.
 * @param session Value supplied for `session`.
 */
void CompilationController::compileAstIntoSession(ast::NodePtr root, const CompilationOptions &options, compilation::CompilationSession &session) const {
    validateRunOptions(options);
    if (!root) {
        throw std::runtime_error("CompilationController: AST root cannot be null");
    }

    session.artifacts().set<ast::NodePtr>(options_.artifacts.ast, std::move(root));

    if (options.validateAst) {
        const ast::NodePtr &currentRoot{session.artifacts().require<ast::NodePtr>(options_.artifacts.ast)};
        if (!currentRoot) {
            throw std::runtime_error("CompilationController: AST root is null during validation");
        }
        requireStep(steps_.validateAst, "validateAst")(*currentRoot, session);
    }

    runStage(options_.stages.ast, session);

    if (options.produceHIR) {
        const ast::NodePtr &currentRoot{session.artifacts().require<ast::NodePtr>(options_.artifacts.ast)};
        if (!currentRoot) {
            throw std::runtime_error("CompilationController: AST artifact is null before HIR lowering");
        }

        session.artifacts().set<ir::HIRModule>(    options_.artifacts.hir,     requireStep(steps_.lowerHIR, "lowerHIR")(*currentRoot, session));
        runStage(options_.stages.hir, session);
    }

    if (options.produceMIR) {
        const ir::HIRModule &currentHIR{session.artifacts().require<ir::HIRModule>(options_.artifacts.hir)};
        session.artifacts().set<ir::MIRModule>(    options_.artifacts.mir,     requireStep(steps_.lowerMIR, "lowerMIR")(currentHIR, session));
        runStage(options_.stages.mir, session);
    }

    runBackend(options, session);
}

/**
 * @brief Implements the `runBackend` operation.
 *
 * @param options Value supplied for `options`.
 * @param session Value supplied for `session`.
 */
void CompilationController::runBackend(const CompilationOptions &options, compilation::CompilationSession &session) const {
    const std::string selected{backendFor(options)};
    if (selected.empty()) {
        return;
    }

    runStage(options_.stages.beforeBackend, session);
    backends_.run(selected, session);
    runStage(options_.stages.afterBackend, session);
}

/**
 * @brief Implements the `compile` operation.
 *
 * @param sourceText Value supplied for `sourceText`.
 * @return Value produced by the operation.
 */
compilation::CompilationSession CompilationController::compile(
    const std::string &sourceText) const {
    return compile(sourceText, options_.defaults);
}

/**
 * @brief Implements the `compile` operation.
 *
 * @param sourceText Value supplied for `sourceText`.
 * @param options Value supplied for `options`.
 * @return Value produced by the operation.
 */
compilation::CompilationSession CompilationController::compile(const std::string &sourceText, CompilationOptions options) const {
    validateRunOptions(options);
    compilation::CompilationSession session{};
    session.artifacts().set<std::string>(options_.artifacts.source, sourceText);

    std::vector<token::Token> tokens{requireStep(steps_.tokenize, "tokenize")(sourceText, std::nullopt, session)};
    ast::NodePtr root{parseTokensIntoSession(std::move(tokens), options, session)};
    compileAstIntoSession(std::move(root), options, session);
    return session;
}

/**
 * @brief Implements the `compileSourceIntoSession` operation.
 *
 * @param sourceValue Value supplied for `sourceValue`.
 * @param options Value supplied for `options`.
 * @param session Value supplied for `session`.
 */
void CompilationController::compileSourceIntoSession( const source::Source &sourceValue, const CompilationOptions &options, compilation::CompilationSession &session) const {
    validateRunOptions(options);
    session.artifacts().set<source::Source>(options_.artifacts.source, sourceValue);

    source::PreprocessedSource processed{requireStep(steps_.preprocess, "preprocess")(sourceValue, options.preprocess, session)};
    session.artifacts().set<source::PreprocessedSource>(options_.artifacts.preprocessedSource, processed);

    std::vector<token::Token> tokens{};
    for (const source::SourceFragment &fragment : processed.fragments) {
        std::vector<token::Token> fragmentTokens{requireStep(steps_.tokenize, "tokenize")(fragment.text, fragment.origin, session)};
        if (!fragmentTokens.empty()) {
            fragmentTokens.pop_back();
        }
        tokens.insert(    tokens.end(),     std::make_move_iterator(fragmentTokens.begin()),     std::make_move_iterator(fragmentTokens.end()));
    }

    const diagnostics::SourceLocation &endLocation{processed.terminalLocation};
    tokens.push_back({token::Kind::End, "", "", {endLocation, endLocation}, endLocation.line, endLocation.column});

    ast::NodePtr root{parseTokensIntoSession(std::move(tokens), options, session)};
    compileAstIntoSession(std::move(root), options, session);
}

/**
 * @brief Implements the `compile` operation.
 *
 * @param sourceValue Value supplied for `sourceValue`.
 * @return Value produced by the operation.
 */
compilation::CompilationSession CompilationController::compile(const source::Source &sourceValue) const {
    return compile(sourceValue, options_.defaults);
}

/**
 * @brief Implements the `compile` operation.
 *
 * @param sourceValue Value supplied for `sourceValue`.
 * @param options Value supplied for `options`.
 * @return Value produced by the operation.
 */
compilation::CompilationSession CompilationController::compile(const source::Source &sourceValue, CompilationOptions options) const {
    compilation::CompilationSession session{};
    compileSourceIntoSession(sourceValue, options, session);
    return session;
}

/**
 * @brief Implements the `compileResolved` operation.
 *
 * @param specifier Value supplied for `specifier`.
 * @return Value produced by the operation.
 */
compilation::CompilationSession CompilationController::compileResolved(const std::string &specifier) const {
    return compileResolved(specifier, options_.defaults);
}

/**
 * @brief Implements the `compileResolved` operation.
 *
 * @param specifier Value supplied for `specifier`.
 * @param options Value supplied for `options`.
 * @return Value produced by the operation.
 */
compilation::CompilationSession CompilationController::compileResolved( const std::string &specifier, CompilationOptions options) const {
    if (specifier.empty()) {
        throw std::runtime_error("CompilationController::compileResolved: source specifier cannot be empty");
    }

    compilation::CompilationSession session{};
    source::Source sourceValue{requireStep(steps_.resolveSource, "resolveSource")(specifier, session)};
    compileSourceIntoSession(sourceValue, options, session);
    return session;
}

/**
 * @brief Implements the `compileTokens` operation.
 *
 * @param tokens Value supplied for `tokens`.
 * @return Value produced by the operation.
 */
compilation::CompilationSession CompilationController::compileTokens(std::vector<token::Token> tokens) const {
    return compileTokens(std::move(tokens), options_.defaults);
}

/**
 * @brief Implements the `compileTokens` operation.
 *
 * @param tokens Value supplied for `tokens`.
 * @param options Value supplied for `options`.
 * @return Value produced by the operation.
 */
compilation::CompilationSession CompilationController::compileTokens(std::vector<token::Token> tokens, CompilationOptions options) const {
    validateRunOptions(options);
    compilation::CompilationSession session{};
    ast::NodePtr root{parseTokensIntoSession(std::move(tokens), options, session)};
    compileAstIntoSession(std::move(root), options, session);
    return session;
}

/**
 * @brief Implements the `compile` operation.
 *
 * @param root Value supplied for `root`.
 * @return Value produced by the operation.
 */
compilation::CompilationSession CompilationController::compile(ast::NodePtr root) const {
    return compile(std::move(root), options_.defaults);
}

/**
 * @brief Implements the `compile` operation.
 *
 * @param root Value supplied for `root`.
 * @param options Value supplied for `options`.
 * @return Value produced by the operation.
 */
compilation::CompilationSession CompilationController::compile(ast::NodePtr root, CompilationOptions options) const {
    compilation::CompilationSession session{};
    compileAstIntoSession(std::move(root), options, session);
    return session;
}

} // namespace novac::controllers
