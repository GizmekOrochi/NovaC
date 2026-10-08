#pragma once

#include "../EngineController.hpp"

#include <cstddef>
#include <optional>
#include <string>

namespace novac::controlflow {

/** @brief Controls whether statement() also installs the trigger in the lexer. */
enum class TriggerRegistration {
    /** The trigger is already tokenized by another registration. */
    Existing,

    /** Register the trigger as a lexer keyword. */
    Keyword,

    /** Register the trigger as a lexer symbol. */
    Symbol
};

/**
 * @brief Hooks composing one language-defined control-flow statement.
 *
 * The specification is converted to a regular EngineFeature installer, so
 * custom control flow participates in the same transactional installation as
 * every other NovaC language feature.
 */
struct StatementSpec {
    /** Parser domain receiving the statement rule. */
    ids::ParseDomain domain{ids::ParseDomain{""}};

    /** Parser-rule key and optional lexer trigger text. */
    std::string trigger{};

    /** Whether the trigger is existing, a keyword, or a symbol. */
    TriggerRegistration triggerRegistration{TriggerRegistration::Existing};

    /** AST schema registered for the statement node. */
    ast::NodeSchema schema{};

    /** Parser callback constructing the statement node. */
    parser::ParseFn parse{};

    /** Optional runtime statement handler. */
    std::optional<runtime::StmtHandler> runtime{};

    /** Optional AST-to-HIR lowerer. */
    std::optional<ir::LoweringRegistry::HIRLowerer> hir{};
};

/**
 * @brief Builds an EngineFeature installer from a control-flow statement spec.
 *
 * The returned installer performs the requested lexer registration, then
 * installs the AST schema, parser rule, optional runtime handler, and optional
 * HIR lowerer. Use it with EngineFeature::onInstall() so the complete statement
 * participates in normal transactional feature installation.
 *
 * @param spec Statement registrations to compose.
 * @return Installer suitable for EngineFeature::onInstall().
 * @throws std::runtime_error If the domain, trigger, schema kind, or parser
 * callback is missing.
 */
controllers::EngineFeature::Installer statement(StatementSpec spec);

/**
 * @brief Reusable iteration guard for language-defined loops.
 *
 * A maximum of zero means unlimited execution. Call step() immediately before
 * each body execution.
 */
class LoopGuard final {
public:
    /**
     * @brief Constructs a `LoopGuard` instance.
     *
     * @param maximumIterations Value supplied for `maximumIterations`.
     */
    explicit LoopGuard(std::size_t maximumIterations = 0);

    /**
     * @brief Records one loop-body execution.
     * @throws std::runtime_error If the configured maximum has already been reached.
     */
    void step();

    /** @brief Returns the number of accepted body executions. */
    std::size_t iterations() const noexcept;

    /** @brief Returns the configured maximum, where zero means unlimited. */
    std::size_t maximumIterations() const noexcept;

private:
    std::size_t maximumIterations_{};
    std::size_t iterations_{};
};

} // namespace novac::controlflow
