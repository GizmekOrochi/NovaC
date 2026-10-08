#pragma once

#include "Preprocessor.hpp"
#include "SourceController.hpp"
#include "../foundation/Diagnostic.hpp"
#include "../foundation/registry/Registry.hpp"
#include "../syntax/Lexer.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace novac::source {

class PreprocessorController;

/**
 * @brief Mutable state exposed to one directive while preprocessing a source.
 *
 * Context instances are short-lived views over one preprocessing session. All
 * includes/imports share defines, conditional state, imported-source tracking,
 * diagnostics and output fragments within that session.
 */
class PreprocessorContext {
public:
    /** @brief Returns the source currently being scanned. */
    const Source &source() const;

    /** @brief Returns the diagnostic engine shared by this preprocessing session. */
    diagnostics::DiagnosticEngine &diagnostics();

    /** @brief Defines a symbol for the remainder of the preprocessing session. */
    void define(std::string name);

    /** @brief Removes a symbol from the preprocessing session. */
    void undefine(const std::string &name);

    /** @brief Returns true when a symbol is currently defined. */
    bool defined(const std::string &name) const;

    /** @brief Returns whether ordinary text/directives are active in the current branch. */
    bool active() const;

    /** @brief Opens a nested conditional branch. */
    void pushCondition(bool value);

    /** @brief Switches the innermost conditional to its alternate branch. */
    void alternateCondition();

    /** @brief Closes the innermost conditional opened in the current source. */
    void popCondition();

    /** @brief Resolves and preprocesses another source every time it is requested. */
    void include(const std::string &specifier);

    /** @brief Resolves and preprocesses a canonical source at most once per import session. */
    void import(const std::string &specifier);

    /** Emits generated text while retaining an explicit source origin. */
    void emit(std::string text, diagnostics::SourceLocation origin);

private:
    friend class PreprocessorController;

    struct ConditionalFrame {
        bool parentActive{};
        bool condition{};
        bool alternateSeen{};
    };

    using IncludeCallback = std::function<void(const std::string &, bool)>;

    /**
     * @brief Constructs a `PreprocessorContext` instance.
     *
     * @param source Value supplied for `source`.
     * @param diagnostics Value supplied for `diagnostics`.
     * @param defines Value supplied for `defines`.
     * @param conditionals Value supplied for `conditionals`.
     * @param conditionalFloor Value supplied for `conditionalFloor`.
     * @param output Value supplied for `output`.
     * @param includeCallback Value supplied for `includeCallback`.
     */
    PreprocessorContext(
        const Source &source,
        diagnostics::DiagnosticEngine &diagnostics,
        std::unordered_set<std::string> &defines,
        std::vector<ConditionalFrame> &conditionals,
        std::size_t conditionalFloor,
        std::vector<SourceFragment> &output,
        IncludeCallback includeCallback);

    const Source *source_{};
    diagnostics::DiagnosticEngine *diagnostics_{};
    std::unordered_set<std::string> *defines_{};
    std::vector<ConditionalFrame> *conditionals_{};
    std::size_t conditionalFloor_{};
    std::vector<SourceFragment> *output_{};
    IncludeCallback includeCallback_{};
};

/**
 * @brief Extensible source preprocessing controller.
 *
 * Directive handlers are configuration; all per-compilation state lives in a
 * PreprocessorContext created by process().
 */
class PreprocessorController {
public:
    using DirectiveHandler = std::function<void(const Directive &, PreprocessorContext &)>;
    using PragmaHandler = std::function<void(const Directive &, PreprocessorContext &)>;

    /**
     * @brief Constructs a `PreprocessorController` instance.
     *
     * @param duplicatePolicy Value supplied for `duplicatePolicy`.
     */
    explicit PreprocessorController(
        registry::DuplicatePolicy duplicatePolicy = registry::DuplicatePolicy::Error);

    /**
     * @brief Registers a source directive handler.
     *
     * The name excludes the directive prefix. The name "pragma" is reserved
     * for named pragma dispatch and must be registered with pragma().
     *
     * @param name Directive name.
     * @param handler Directive callback.
     * @param mode Whether the callback runs in inactive conditional branches.
     * @return Registration result.
     * @throws std::runtime_error If the name/handler is empty, "pragma" is used,
     * or duplicate registration is rejected.
     */
    registry::RegisterStatus directive(
        std::string name,
        DirectiveHandler handler,
        DirectiveMode mode = DirectiveMode::ActiveOnly);

    /**
     * @brief Registers a named handler dispatched by #pragma.
     *
     * A source directive ``#pragma NAME rest`` dispatches NAME and passes
     * ``rest`` as Directive::arguments to the registered callback.
     *
     * @param name Pragma name.
     * @param handler Pragma callback.
     * @return Registration result.
     */
    registry::RegisterStatus pragma(std::string name, PragmaHandler handler);

    /**
     * @brief Preprocesses one root source before language lexing.
     *
     * Per-run defines, import state, conditional state, source stack, and output
     * fragments are created for this call. The lexer registry is consulted only
     * for line/block comment delimiters used while recognizing directives.
     *
     * @param source Root logical source.
     * @param sources Source resolver controller used by include/import.
     * @param lexerRules Lexer configuration supplying comment delimiters.
     * @param diagnostics Diagnostic engine shared with the caller.
     * @param options Per-run preprocessing options.
     * @return Source fragments preserving their original origins.
     * @throws std::runtime_error For malformed directives, unresolved sources,
     * cycles/depth overflow, unmatched conditionals, or unknown active directives/pragmas.
     */
    PreprocessedSource process(
        const Source &source,
        const SourceController &sources,
        const lexer::LexerRegistry &lexerRules,
        diagnostics::DiagnosticEngine &diagnostics,
        PreprocessOptions options = {}) const;

private:
    struct DirectiveEntry {
        DirectiveHandler handler{};
        DirectiveMode mode{DirectiveMode::ActiveOnly};
    };

    std::unordered_map<std::string, DirectiveEntry> directives_{};
    std::unordered_map<std::string, PragmaHandler> pragmas_{};
    registry::DuplicatePolicy duplicatePolicy_;
};

} // namespace novac::source
