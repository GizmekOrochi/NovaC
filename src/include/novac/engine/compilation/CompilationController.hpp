#pragma once

#include "novac/engine/EngineController.hpp"
#include "novac/engine/backend/Backend.hpp"
#include "novac/engine/compilation/Compilation.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace novac::controllers {

/**
 * @brief Stable artifact names used by the optional classic compilation facade.
 *
 * These names are only defaults for CompilationController. The generic
 * CompilationSession/ArtifactStore APIs do not reserve any artifact names.
 */
struct CompilationArtifactNames {
    std::string source{"source"};
    std::string preprocessedSource{"preprocessed-source"};
    std::string tokens{"tokens"};
    std::string ast{"ast"};
    std::string hir{"hir"};
    std::string mir{"mir"};
};

/**
 * @brief Pass stages exposed by the optional classic compilation facade.
 *
 * Stage names are ordinary strings and can be replaced or disabled by setting
 * them to an empty string. PassRegistry itself remains independent from these
 * names and can be used without CompilationController.
 */
struct CompilationStageNames {
    std::string tokens{"tokens"};
    std::string ast{"ast"};
    std::string hir{"hir"};
    std::string mir{"mir"};
    std::string beforeBackend{"before-backend"};
    std::string afterBackend{"after-backend"};
};

/**
 * @brief Per-run options for NovaC's convenience source -> AST -> HIR -> MIR path.
 *
 * These switches belong only to CompilationController. They do not define or
 * constrain NovaC's generic compilation model.
 */
struct CompilationOptions {
    /** Validate the parsed/provided AST. */
    bool validateAst{true};

    /** Produce the legacy CFG-oriented HIR artifact. */
    bool produceHIR{true};

    /** Produce the legacy CFG-oriented MIR artifact. Requires produceHIR. */
    bool produceMIR{true};

    /** Optional parser entry domain. Empty means the controller default. */
    std::string startDomain{};

    /** Optional backend id. Empty means CompilationControllerOptions::defaultBackend. */
    std::string backend{};

    /** Options used only when compiling a logical source through preprocessing. */
    source::PreprocessOptions preprocess{};
};

/**
 * @brief Construction options for the optional classic compilation facade.
 */
struct CompilationControllerOptions {
    registry::DuplicatePolicy duplicatePolicy{registry::DuplicatePolicy::Error};
    CompilationArtifactNames artifacts{};
    CompilationStageNames stages{};

    /**
     * Default parser entry domain used when a run does not provide one.
     *
     * When empty, CompilationSteps::defaultStartDomain is queried. The standard
     * EngineController-backed steps use EngineController::startDomain().
     */
    std::string defaultStartDomain{};

    /** Backend used when a run does not explicitly select another backend. */
    std::string defaultBackend{};

    /**
     * Default run configuration used by compile(...) overloads that do not
     * receive explicit CompilationOptions.
     */
    CompilationOptions defaults{};
};

/**
 * @brief Replaceable operations used by CompilationController.
 *
 * CompilationController is intentionally a recipe, not a compiler core. Every
 * operation that gives the classic recipe its meaning lives in this structure.
 * The EngineController constructor fills it with NovaC's standard lexer,
 * parser, AST validation and HIR/MIR lowering behavior. Users can replace one
 * callback, provide all callbacks themselves, or bypass CompilationController
 * entirely and compose the low-level APIs exposed by <NovaC.hpp>.
 */
struct CompilationSteps {
    using ResolveSourceFn = std::function<source::Source(const std::string &specifier, compilation::CompilationSession &session)>;
    using PreprocessFn = std::function<source::PreprocessedSource(const source::Source &sourceValue, const source::PreprocessOptions &options, compilation::CompilationSession &session)>;
    using TokenizeFn = std::function<std::vector<token::Token>(const std::string &text, std::optional<diagnostics::SourceLocation> origin, compilation::CompilationSession &session)>;
    using ParseFn = std::function<ast::NodePtr(std::vector<token::Token> tokens, const std::string &startDomain, compilation::CompilationSession &session)>;
    using ValidateAstFn = std::function<void(const ast::Node &root, compilation::CompilationSession &session)>;
    using LowerHIRFn = std::function<ir::HIRModule(const ast::Node &root, compilation::CompilationSession &session)>;
    using LowerMIRFn = std::function<ir::MIRModule(const ir::HIRModule &hir, compilation::CompilationSession &session)>;
    using DefaultStartDomainFn = std::function<std::string()>;

    ResolveSourceFn resolveSource{};
    PreprocessFn preprocess{};
    TokenizeFn tokenize{};
    ParseFn parse{};
    ValidateAstFn validateAst{};
    LowerHIRFn lowerHIR{};
    LowerMIRFn lowerMIR{};
    DefaultStartDomainFn defaultStartDomain{};
};

/**
 * @brief Creates the standard classic-compilation callbacks for an engine.
 *
 * The returned callbacks borrow @p engine. The engine must therefore outlive
 * every controller or callback set created from this function.
 */
CompilationSteps classicCompilationSteps(EngineController &engine);

/**
 * @brief Optional convenience controller for NovaC's historical compilation path.
 *
 * This class deliberately contains no mandatory language semantics. It runs a
 * configurable recipe using CompilationSteps, generic PassRegistry stages and
 * an optional BackendRegistry.
 *
 * Typical/default use:
 * @code
 * EngineController engine{{.startDomain = "program"}};
 * CompilationController compiler{engine};
 * auto session = compiler.compile(sourceText);
 * @endcode
 *
 * Partial replacement:
 * @code
 * auto steps = classicCompilationSteps(engine);
 * steps.lowerHIR = myLowering;
 * CompilationController compiler{std::move(steps)};
 * @endcode
 *
 * Fully custom compiler architectures do not need this class at all: use the
 * source/syntax/semantic/pass/IR/backend primitives from <NovaC.hpp> directly.
 */
class CompilationController final {
public:
    /**
     * @brief Creates the classic controller wired to an EngineController.
     *
     * All classic operations receive standard implementations. No extra header
     * is required: <NovaC.hpp> exposes this controller and all low-level pieces.
     */
    explicit CompilationController(EngineController &engine, CompilationControllerOptions options = {});

    /**
     * @brief Creates the same convenience facade from user-provided operations.
     *
     * Only callbacks actually needed by a requested compile overload/stage must
     * be supplied. Missing callbacks produce a targeted runtime error.
     */
    explicit CompilationController(CompilationSteps steps, CompilationControllerOptions options = {});

    /** Register a pass consumed by one of the configured stage names. */
    registry::RegisterStatus pass(compilation::PassDescriptor descriptor);

    /** Register a backend for optional final emission. */
    registry::RegisterStatus backend(std::string id, std::shared_ptr<const novac::backend::Backend> implementation);

    /**
     * @brief Performs the `passes` operation.
     *
     * @return Value produced by the operation.
     */
    compilation::PassRegistry &passes() noexcept;
    /**
     * @brief Performs the `passes` operation.
     *
     * @return Value produced by the operation.
     */
    const compilation::PassRegistry &passes() const noexcept;

    /**
     * @brief Performs the `backends` operation.
     *
     * @return Value produced by the operation.
     */
    novac::backend::BackendRegistry &backends() noexcept;
    /**
     * @brief Performs the `backends` operation.
     *
     * @return Value produced by the operation.
     */
    const novac::backend::BackendRegistry &backends() const noexcept;

    /** Returns the replaceable classic operations. */
    CompilationSteps &steps() noexcept;
    /**
     * @brief Performs the `steps` operation.
     *
     * @return Value produced by the operation.
     */
    const CompilationSteps &steps() const noexcept;

    /** True when this controller was constructed from an EngineController. */
    bool hasEngine() const noexcept;

    /**
     * Returns the borrowed engine.
     * @throws std::runtime_error If this controller was built only from custom steps.
     */
    EngineController &engine();
    /**
     * @brief Returns the value exposed by `engine`.
     *
     * @return Value produced by the operation.
     */
    const EngineController &engine() const;

    /**
     * @brief Performs the `options` operation.
     *
     * @return Value produced by the operation.
     */
    const CompilationControllerOptions &options() const noexcept;

    /** Compile plain source text using the controller's default run options. */
    compilation::CompilationSession compile(const std::string &sourceText) const;

    /** Compile plain source text with run-specific options. */
    compilation::CompilationSession compile(const std::string &sourceText, CompilationOptions options) const;

    /** Compile a logical source using the controller's default run options. */
    compilation::CompilationSession compile(const source::Source &sourceValue) const;

    /** Compile a logical source with run-specific options. */
    compilation::CompilationSession compile(const source::Source &sourceValue, CompilationOptions options) const;

    /** Resolve and compile a logical source using the controller defaults. */
    compilation::CompilationSession compileResolved(const std::string &specifier) const;

    /** Resolve and compile a logical source with run-specific options. */
    compilation::CompilationSession compileResolved(const std::string &specifier, CompilationOptions options) const;

    /** Compile already-tokenized input using the controller defaults. */
    compilation::CompilationSession compileTokens(std::vector<token::Token> tokens) const;

    /** Compile already-tokenized input with run-specific options. */
    compilation::CompilationSession compileTokens(std::vector<token::Token> tokens, CompilationOptions options) const;

    /** Compile an AST root using the controller defaults. */
    compilation::CompilationSession compile(ast::NodePtr root) const;

    /** Compile an AST root with run-specific options. */
    compilation::CompilationSession compile(ast::NodePtr root, CompilationOptions options) const;

private:
    /**
     * @brief Starts the operation represented by `startDomainFor`.
     *
     * @param options Value supplied for `options`.
     * @return Value produced by the operation.
     */
    std::string startDomainFor(const CompilationOptions &options) const;
    /**
     * @brief Performs the `backendFor` operation.
     *
     * @param options Value supplied for `options`.
     * @return Value produced by the operation.
     */
    std::string backendFor(const CompilationOptions &options) const;

    /**
     * @brief Performs the `runStage` operation.
     *
     * @param stage Value supplied for `stage`.
     * @param session Value supplied for `session`.
     */
    void runStage(const std::string &stage, compilation::CompilationSession &session) const;
    /**
     * @brief Parses input through `parseTokensIntoSession`.
     *
     * @param tokens Value supplied for `tokens`.
     * @param options Value supplied for `options`.
     * @param session Value supplied for `session`.
     * @return Value produced by the operation.
     */
    ast::NodePtr parseTokensIntoSession(std::vector<token::Token> tokens, const CompilationOptions &options, compilation::CompilationSession &session) const;
    /**
     * @brief Performs the `compileSourceIntoSession` operation.
     *
     * @param sourceValue Value supplied for `sourceValue`.
     * @param options Value supplied for `options`.
     * @param session Value supplied for `session`.
     */
    void compileSourceIntoSession(const source::Source &sourceValue, const CompilationOptions &options, compilation::CompilationSession &session) const;
    /**
     * @brief Performs the `compileAstIntoSession` operation.
     *
     * @param root Value supplied for `root`.
     * @param options Value supplied for `options`.
     * @param session Value supplied for `session`.
     */
    void compileAstIntoSession(ast::NodePtr root, const CompilationOptions &options, compilation::CompilationSession &session) const;
    /**
     * @brief Performs the `runBackend` operation.
     *
     * @param options Value supplied for `options`.
     * @param session Value supplied for `session`.
     */
    void runBackend(const CompilationOptions &options, compilation::CompilationSession &session) const;

    EngineController *engine_{nullptr};
    CompilationControllerOptions options_{};
    CompilationSteps steps_{};
    compilation::PassRegistry passes_;
    novac::backend::BackendRegistry backends_;
};

} // namespace novac::controllers
