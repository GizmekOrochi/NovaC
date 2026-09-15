#pragma once

#include "novac/assets/essentials/EssentialFeature.hpp"
#include "novac/assets/essentials/functions/FunctionRegistry.hpp"
#include "novac/engine/EngineController.hpp"

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace novac::assets::essentials {

/**
 * @brief Configures the shared syntax used by Essentials features.
 *
 * These values define the parser domains, common AST node kinds, shared field
 * names, and punctuation tokens used to connect independently installed
 * statement features.
 */
struct CoreSyntaxOptions {
    std::string programDomain{"program"};
    std::string statementDomain{"stmt"};
    std::string expressionDomain{"expr"};

    std::string programNodeKind{"Program"};
    std::string blockNodeKind{"BlockStmt"};
    std::string expressionStatementNodeKind{"ExpressionStatement"};

    std::string statementsField{"statements"};
    std::string expressionField{"expression"};

    std::string semicolonToken{";"};
    std::string commaToken{","};
    std::string leftBraceToken{"{"};
    std::string rightBraceToken{"}"};
    std::string leftParenToken{"("};
    std::string rightParenToken{")"};
};

/**
 * @brief Configures variable declaration, lookup, and assignment syntax.
 */
struct VariableSyntaxOptions {
    std::string declarationNodeKind{"VariableDeclaration"};
    std::string expressionNodeKind{"VariableExpression"};
    std::string assignmentNodeKind{"AssignmentStatement"};

    std::string letKeyword{"let"};
    std::string assignToken{"="};

    std::string nameField{"name"};
    std::string valueField{"value"};
};

/**
 * @brief Configures conditional and loop syntax.
 *
 * The iteration limit is used by loop runtime handlers as a guard against
 * accidental non-terminating programs.
 */
struct ControlFlowSyntaxOptions {
    std::string ifNodeKind{"IfStatement"};
    std::string whileNodeKind{"WhileStatement"};
    std::string forNodeKind{"ForStatement"};

    std::string ifKeyword{"if"};
    std::string elseKeyword{"else"};
    std::string whileKeyword{"while"};
    std::string forKeyword{"for"};

    std::string conditionField{"condition"};
    std::string thenField{"thenBranch"};
    std::string elseField{"elseBranch"};
    std::string bodyField{"body"};
    std::string initializerField{"initializer"};
    std::string stepField{"step"};

    int maxLoopIterations{100000};
};

/**
 * @brief Configures function declaration, call, and return syntax.
 */
struct FunctionSyntaxOptions {
    std::string declarationNodeKind{"FunctionDeclaration"};
    std::string callNodeKind{"FunctionCall"};
    std::string parameterNodeKind{"FunctionParameter"};
    std::string returnNodeKind{"ReturnStatement"};

    std::string functionKeyword{"func"};
    std::string returnKeyword{"return"};
    std::string mainFunctionName{"main"};

    std::string nameField{"name"};
    std::string bodyField{"body"};
    std::string parametersField{"parameters"};
    std::string argumentsField{"arguments"};
    std::string valueField{"value"};
};

/**
 * @brief Complete configuration for EssentialsController.
 *
 * Groups the syntax configuration of every standard Essentials subsystem and
 * optionally enables semantic child-trait validation in generated schemas.
 */
struct EssentialsControllerOptions {
    CoreSyntaxOptions core{};
    VariableSyntaxOptions variables{};
    ControlFlowSyntaxOptions controlFlow{};
    FunctionSyntaxOptions functions{};

    bool enforceChildTraits{false};
};

/**
 * @brief Installs statement-level language building blocks into an engine.
 *
 * EssentialsController is the integration point for scopes, variables,
 * control flow, functions, and the program root. Features may be installed by
 * reference or transferred through EssentialPack objects. Owned features are
 * kept alive by the controller after installation.
 */
class EssentialsController {
public:
    /**
     * @brief Creates an Essentials controller bound to an engine.
     *
     * Validates the supplied syntax configuration and selects the configured
     * program domain as the engine start domain.
     *
     * @param engine Engine controller configured by this instance. It must
     * outlive the EssentialsController.
     * @param options Syntax and validation configuration.
     * @throws std::runtime_error If a required option is empty or invalid.
     */
    explicit EssentialsController(
        controllers::EngineController &engine,
        EssentialsControllerOptions options = {}
    );

    /**
     * @brief Installs a feature without taking ownership.
     *
     * @param feature Feature to validate and install.
     * @return This controller, for fluent chaining.
     * @throws std::runtime_error If metadata is invalid, duplicated, or a
     * requirement is missing.
     */
    EssentialsController &use(const EssentialFeature &feature);
    /**
     * @brief Installs and takes ownership of every feature in a pack.
     *
     * @param pack Feature pack to consume.
     * @return This controller, for fluent chaining.
     */
    EssentialsController &use(EssentialPack pack);
    /**
     * @brief Installs and stores ownership of a feature.
     *
     * @param feature Feature to own and install.
     * @return This controller, for fluent chaining.
     * @throws std::runtime_error If the pointer is null or the feature is invalid.
     */
    EssentialsController &own(std::unique_ptr<EssentialFeature> feature);

    /**
     * @brief Installs the program root parser and runtime handler.
     *
     * @return This controller, for fluent chaining.
     * @throws std::runtime_error If the program feature is already installed.
     */
    EssentialsController &installProgram();
    /** @brief Installs lexical scoped blocks. */
    EssentialsController &installScopedBlocks();
    /** @brief Installs variable declarations, assignments, and lookups. */
    EssentialsController &installVariables();
    /** @brief Installs standalone expression statements. */
    EssentialsController &installExpressionStatements();
    /** @brief Installs conditional if/else statements. */
    EssentialsController &installIfStatements();
    /** @brief Installs while loops. */
    EssentialsController &installWhileLoops();
    /** @brief Installs C-style for loops. */
    EssentialsController &installForLoops();
    /** @brief Installs function return statements. */
    EssentialsController &installReturnStatements();
    /** @brief Installs function declarations and calls. */
    EssentialsController &installFunctions();

    /** @brief Installs the standard scope feature set. */
    EssentialsController &installStandardScopes();
    /** @brief Installs the standard variable feature set. */
    EssentialsController &installStandardVariables();
    /** @brief Installs the standard control-flow feature set. */
    EssentialsController &installStandardControlFlow();
    /** @brief Installs the standard function feature set. */
    EssentialsController &installStandardFunctions();
    /**
     * @brief Installs the complete standard Essentials language core.
     *
     * Installs the program root, scopes, variables, control flow, and
     * functions, then restores the configured program domain as the engine
     * start domain.
     *
     * @return This controller, for fluent chaining.
     */
    EssentialsController &installStandardCore();

    /** @brief Returns the engine configured by this controller. */
    controllers::EngineController &engine();
    /** @brief Returns the engine configured by this controller. */
    const controllers::EngineController &engine() const;

    /** @brief Returns the complete Essentials configuration. */
    const EssentialsControllerOptions &options() const;
    /** @brief Returns the shared core syntax configuration. */
    const CoreSyntaxOptions &core() const;
    /** @brief Returns the variable syntax configuration. */
    const VariableSyntaxOptions &variables() const;
    /** @brief Returns the control-flow syntax configuration. */
    const ControlFlowSyntaxOptions &controlFlow() const;
    /** @brief Returns the function syntax configuration. */
    const FunctionSyntaxOptions &functions() const;

    /** @brief Returns the mutable native-function registry. */
    functions::FunctionRegistry &functionRegistry();
    /** @brief Returns the native-function registry. */
    const functions::FunctionRegistry &functionRegistry() const;

    /**
     * @brief Tests whether a feature identifier is already installed.
     *
     * @param id Feature identifier to search for.
     * @return true when the feature is registered, otherwise false.
     */
    bool hasFeature(const std::string &id) const;
    /** @brief Returns metadata for every installed feature. */
    const std::vector<EssentialInfo> &features() const;

private:
    void validateOptions() const;
    void validateFeature(const EssentialInfo &info) const;
    void rememberFeature(EssentialInfo info);

    functions::FunctionRegistry functionRegistry_;
    controllers::EngineController &engine_;
    EssentialsControllerOptions options_;
    std::vector<EssentialInfo> features_;
    std::vector<std::unique_ptr<EssentialFeature>> ownedFeatures_;
    std::unordered_set<std::string> featureIds_;
};

namespace essentials {

/**
 * @brief Builds the standard Essentials feature pack.
 *
 * @return Pack containing the standard reusable Essentials features.
 */
EssentialPack standard();

} // namespace essentials

} // namespace novac::assets::essentials
