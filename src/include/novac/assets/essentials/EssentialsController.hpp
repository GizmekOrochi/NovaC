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
 * @brief Configures syntax shared by Essentials features.
 *
 * CoreSyntaxOptions defines the parser domains, common AST node kinds, shared
 * field names and punctuation tokens used to connect independently installable
 * statement-level features.
 */
struct CoreSyntaxOptions {
    /** Parse domain used for complete programs. */
    std::string programDomain{"program"};

    /** Parse domain used for statements. */
    std::string statementDomain{"stmt"};

    /** Parse domain used for expressions. */
    std::string expressionDomain{"expr"};

    /** AST node kind used for the program root. */
    std::string programNodeKind{"Program"};

    /** AST node kind used for lexical blocks. */
    std::string blockNodeKind{"BlockStmt"};

    /** AST node kind used for expression statements. */
    std::string expressionStatementNodeKind{"ExpressionStatement"};

    /** Field name containing ordered program or block statements. */
    std::string statementsField{"statements"};

    /** Field name containing an expression child. */
    std::string expressionField{"expression"};

    /** Statement terminator token. */
    std::string semicolonToken{";"};

    /** List separator token. */
    std::string commaToken{","};

    /** Opening block token. */
    std::string leftBraceToken{"{"};

    /** Closing block token. */
    std::string rightBraceToken{"}"};

    /** Opening parenthesis token. */
    std::string leftParenToken{"("};

    /** Closing parenthesis token. */
    std::string rightParenToken{")"};
};

/**
 * @brief Configures variable declaration, lookup and assignment syntax.
 */
struct VariableSyntaxOptions {
    /** AST node kind used for variable declarations. */
    std::string declarationNodeKind{"VariableDeclaration"};

    /** AST node kind used for variable lookup expressions. */
    std::string expressionNodeKind{"VariableExpression"};

    /** AST node kind used for assignment statements. */
    std::string assignmentNodeKind{"AssignmentStatement"};

    /** Keyword introducing a variable declaration. */
    std::string letKeyword{"let"};

    /** Token used for assignment. */
    std::string assignToken{"="};

    /** Field name storing variable identifiers. */
    std::string nameField{"name"};

    /** Field name storing declaration or assignment values. */
    std::string valueField{"value"};
};

/**
 * @brief Configures conditional and loop syntax.
 *
 * maxLoopIterations is used by while and for runtime handlers as a safety
 * guard. Values greater than zero enable the limit; non-positive values disable
 * it.
 */
struct ControlFlowSyntaxOptions {
    /** AST node kind used for if statements. */
    std::string ifNodeKind{"IfStatement"};

    /** AST node kind used for while loops. */
    std::string whileNodeKind{"WhileStatement"};

    /** AST node kind used for for loops. */
    std::string forNodeKind{"ForStatement"};

    /** Keyword introducing an if statement. */
    std::string ifKeyword{"if"};

    /** Keyword introducing an else branch. */
    std::string elseKeyword{"else"};

    /** Keyword introducing a while loop. */
    std::string whileKeyword{"while"};

    /** Keyword introducing a for loop. */
    std::string forKeyword{"for"};

    /** Field name storing a conditional expression. */
    std::string conditionField{"condition"};

    /** Field name storing the then branch. */
    std::string thenField{"thenBranch"};

    /** Field name storing the optional else branch. */
    std::string elseField{"elseBranch"};

    /** Field name storing a loop body. */
    std::string bodyField{"body"};

    /** Field name storing an optional for-loop initializer. */
    std::string initializerField{"initializer"};

    /** Field name storing an optional for-loop step statement. */
    std::string stepField{"step"};

    /** Maximum loop iterations before a runtime error is raised. */
    int maxLoopIterations{100000};
};

/**
 * @brief Configures function declaration, call and return syntax.
 */
struct FunctionSyntaxOptions {
    /** AST node kind used for function declarations. */
    std::string declarationNodeKind{"FunctionDeclaration"};

    /** AST node kind used for function calls. */
    std::string callNodeKind{"FunctionCall"};

    /** AST node kind used for function parameters. */
    std::string parameterNodeKind{"FunctionParameter"};

    /** AST node kind used for return statements. */
    std::string returnNodeKind{"ReturnStatement"};

    /** Keyword introducing a function declaration. */
    std::string functionKeyword{"func"};

    /** Keyword introducing a return statement. */
    std::string returnKeyword{"return"};

    /** Function name treated as the default program entry point. */
    std::string mainFunctionName{"main"};

    /** Field name storing function or parameter names. */
    std::string nameField{"name"};

    /** Field name storing a function body. */
    std::string bodyField{"body"};

    /** Field name storing function parameter nodes. */
    std::string parametersField{"parameters"};

    /** Field name storing function call arguments. */
    std::string argumentsField{"arguments"};

    /** Field name storing return values. */
    std::string valueField{"value"};
};

/**
 * @brief Complete configuration for EssentialsController.
 *
 * Groups the shared core syntax and the configuration of variables, control
 * flow and functions.
 *
 * When enforceChildTraits is enabled, generated AST schemas attach semantic
 * trait restrictions to child fields where supported.
 */
struct EssentialsControllerOptions {
    CoreSyntaxOptions core{};
    VariableSyntaxOptions variables{};
    ControlFlowSyntaxOptions controlFlow{};
    FunctionSyntaxOptions functions{};

    /** Enables semantic child-trait restrictions in generated schemas. */
    bool enforceChildTraits{false};
};

/**
 * @brief Installs statement-level language building blocks into an engine.
 *
 * EssentialsController is the integration point for the reusable Essentials
 * layer: program roots, lexical scopes, variables, control flow and functions.
 *
 * Features may be installed by reference, transferred through EssentialPack,
 * or moved directly into the controller. Owned features are kept alive after
 * installation.
 *
 * The controller references an external EngineController, which must outlive
 * this object.
 */
class EssentialsController {
public:
    /**
     * @brief Creates an Essentials controller bound to an engine.
     *
     * All required string configuration values are validated before use. The
     * configured program domain is then selected as the engine's default parser
     * start domain.
     *
     * @param engine Engine controller configured by this instance. It must
     * outlive the EssentialsController.
     * @param options Syntax and validation configuration.
     *
     * @throws std::runtime_error If a required configuration value is empty.
     */
    explicit EssentialsController(
        controllers::EngineController &engine,
        EssentialsControllerOptions options = {}
    );

    /**
     * @brief Installs a feature without taking ownership.
     *
     * The feature metadata is read and validated before installation. After a
     * successful install, its metadata is recorded by the controller.
     *
     * The caller remains responsible for the lifetime of the feature object.
     *
     * @param feature Feature to validate and install.
     * @return This controller, for fluent chaining.
     *
     * @throws std::runtime_error If the feature id is empty or already installed.
     */
    EssentialsController &use(const EssentialFeature &feature);

    /**
     * @brief Installs and takes ownership of every feature in a pack.
     *
     * Each feature is moved into own() in pack order.
     *
     * @param pack Feature pack to consume.
     * @return This controller, for fluent chaining.
     */
    EssentialsController &use(EssentialPack pack);

    /**
     * @brief Installs and stores ownership of a feature.
     *
     * The feature is validated and installed before ownership is stored.
     *
     * @param feature Feature to own and install.
     * @return This controller, for fluent chaining.
     *
     * @throws std::runtime_error If the pointer is null, the feature id is
     * empty, or the feature is already installed.
     */
    EssentialsController &own(std::unique_ptr<EssentialFeature> feature);

    /**
     * @brief Installs the program root parser and runtime behavior.
     *
     * The program parser repeatedly parses statements until the token stream
     * reaches End and stores them in the configured program node.
     *
     * During evaluation, top-level function declarations are bound first. If a
     * function matching mainFunctionName exists, it is invoked with no
     * arguments and its value becomes the program result. Otherwise top-level
     * nodes are executed sequentially and a pending return value is propagated.
     *
     * @return This controller, for fluent chaining.
     *
     * @throws std::runtime_error If the program feature is already installed.
     */
    EssentialsController &installProgram();

    /**
     * @brief Installs lexical scoped blocks.
     *
     * @return This controller, for fluent chaining.
     */
    EssentialsController &installScopedBlocks();

    /**
     * @brief Installs variable declarations, assignments and lookups.
     *
     * @return This controller, for fluent chaining.
     */
    EssentialsController &installVariables();

    /**
     * @brief Installs standalone expression statements.
     *
     * @return This controller, for fluent chaining.
     */
    EssentialsController &installExpressionStatements();

    /**
     * @brief Installs conditional if/else statements.
     *
     * @return This controller, for fluent chaining.
     */
    EssentialsController &installIfStatements();

    /**
     * @brief Installs while loops.
     *
     * @return This controller, for fluent chaining.
     */
    EssentialsController &installWhileLoops();

    /**
     * @brief Installs C-style for loops.
     *
     * @return This controller, for fluent chaining.
     */
    EssentialsController &installForLoops();

    /**
     * @brief Installs function return statements.
     *
     * @return This controller, for fluent chaining.
     */
    EssentialsController &installReturnStatements();

    /**
     * @brief Installs function declarations and calls.
     *
     * @return This controller, for fluent chaining.
     */
    EssentialsController &installFunctions();

    /**
     * @brief Installs the standard scope feature set.
     *
     * Currently installs scoped blocks.
     *
     * @return This controller, for fluent chaining.
     */
    EssentialsController &installStandardScopes();

    /**
     * @brief Installs the standard variable feature set.
     *
     * Includes variable support and expression statements.
     *
     * @return This controller, for fluent chaining.
     */
    EssentialsController &installStandardVariables();

    /**
     * @brief Installs the standard control-flow feature set.
     *
     * Includes if/else statements, while loops and for loops.
     *
     * @return This controller, for fluent chaining.
     */
    EssentialsController &installStandardControlFlow();

    /**
     * @brief Installs the standard function feature set.
     *
     * Includes return statements, function declarations and calls, and
     * configured entry-point support.
     *
     * @return This controller, for fluent chaining.
     */
    EssentialsController &installStandardFunctions();

    /**
     * @brief Installs the complete standard Essentials language core.
     *
     * Installs the program root, scopes, variables, control flow and functions.
     * The configured program domain is selected again as the engine start domain
     * after all feature installation has completed.
     *
     * @return This controller, for fluent chaining.
     */
    EssentialsController &installStandardCore();

    /**
     * @brief Returns the engine configured by this controller.
     *
     * @return Mutable engine controller.
     */
    controllers::EngineController &engine();

    /**
     * @brief Returns the engine configured by this controller.
     *
     * @return Read-only engine controller.
     */
    const controllers::EngineController &engine() const;

    /**
     * @brief Returns the complete Essentials configuration.
     *
     * @return Controller options.
     */
    const EssentialsControllerOptions &options() const;

    /**
     * @brief Returns the shared core syntax configuration.
     *
     * @return Core syntax options.
     */
    const CoreSyntaxOptions &core() const;

    /**
     * @brief Returns the variable syntax configuration.
     *
     * @return Variable syntax options.
     */
    const VariableSyntaxOptions &variables() const;

    /**
     * @brief Returns the control-flow syntax configuration.
     *
     * @return Control-flow syntax options.
     */
    const ControlFlowSyntaxOptions &controlFlow() const;

    /**
     * @brief Returns the function syntax configuration.
     *
     * @return Function syntax options.
     */
    const FunctionSyntaxOptions &functions() const;

    /**
     * @brief Returns the mutable native-function registry.
     *
     * @return Function registry used by Essentials function calls.
     */
    functions::FunctionRegistry &functionRegistry();

    /**
     * @brief Returns the native-function registry.
     *
     * @return Read-only function registry.
     */
    const functions::FunctionRegistry &functionRegistry() const;

    /**
     * @brief Tests whether a feature identifier is already installed.
     *
     * @param id Feature identifier to search for.
     * @return true when the feature is registered, otherwise false.
     */
    bool hasFeature(const std::string &id) const;

    /**
     * @brief Tests whether an installed Essentials feature provides a capability.
     *
     * @param capability Capability name to search for.
     * @return true when the capability is available, otherwise false.
     */
    bool hasCapability(const std::string &capability) const;

    /**
     * @brief Returns metadata for installed features.
     *
     * Metadata is stored in installation order.
     *
     * @return Installed feature metadata.
     */
    const std::vector<EssentialInfo> &features() const;

private:
    /**
     * @brief Validates required controller configuration values.
     *
     * Every required domain, node kind, field name, keyword and punctuation
     * string is checked for emptiness.
     */
    void validateOptions() const;

    /**
     * @brief Validates feature metadata before installation.
     *
     * Validation requires a non-empty id, rejects duplicate feature ids, and
     * ensures every declared capability requirement is already available.
     */
    void validateFeature(const EssentialInfo &info) const;

    /**
     * @brief Records metadata for a successfully installed feature.
     */
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
 * @brief Builds the standard reusable Essentials feature pack.
 *
 * The pack combines standard scope, variable, control-flow and function
 * features. The program root is not part of this pack and is installed
 * separately by EssentialsController::installProgram() or
 * installStandardCore().
 *
 * @return Pack containing the standard reusable Essentials features.
 */
EssentialPack standard();

} // namespace essentials

} // namespace novac::assets::essentials
