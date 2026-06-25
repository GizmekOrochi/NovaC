#pragma once

#include "novac/engine/EngineController.hpp"

#include <string>
#include <unordered_set>
#include <vector>

namespace novac::assets::essentials {

/**
 * @brief Configuration used by EssentialsController.
 *
 * Essentials does not replace the Engine. It only installs reusable language
 * building blocks on top of an existing EngineController.
 */
struct EssentialsControllerOptions {
    std::string statementDomain{"stmt"};
    std::string expressionDomain{"expr"};

    std::string blockNodeKind{"BlockStmt"};
    std::string statementsField{"statements"};

    std::string variableDeclarationNodeKind{"VariableDeclaration"};
    std::string variableExpressionNodeKind{"VariableExpression"};
    std::string assignmentNodeKind{"AssignmentStatement"};

    std::string expressionStatementNodeKind{"ExpressionStatement"};

    std::string ifNodeKind{"IfStatement"};
    std::string whileNodeKind{"WhileStatement"};
    std::string forNodeKind{"ForStatement"};

    std::string returnNodeKind{"ReturnStatement"};

    std::string functionDeclarationNodeKind{"FunctionDeclaration"};
    std::string functionCallNodeKind{"FunctionCall"};
    std::string parameterNodeKind{"FunctionParameter"};

    std::string nameField{"name"};
    std::string valueField{"value"};
    std::string expressionField{"expression"};
    std::string conditionField{"condition"};
    std::string thenField{"thenBranch"};
    std::string elseField{"elseBranch"};
    std::string bodyField{"body"};
    std::string initializerField{"initializer"};
    std::string stepField{"step"};
    std::string parametersField{"parameters"};
    std::string argumentsField{"arguments"};

    std::string letKeyword{"let"};
    std::string ifKeyword{"if"};
    std::string elseKeyword{"else"};
    std::string whileKeyword{"while"};
    std::string forKeyword{"for"};
    std::string returnKeyword{"return"};
    std::string functionKeyword{"fn"};

    std::string assignToken{"="};
    std::string semicolonToken{";"};
    std::string commaToken{","};
    std::string leftBraceToken{"{"};
    std::string rightBraceToken{"}"};
    std::string leftParenToken{"("};
    std::string rightParenToken{")"};

    /**
     * @brief Enables AST child trait constraints in installed schemas.
     *
     * Keep this false while mixing Essentials with assets that have not yet
     * adopted the shared traits from EssentialTraits.hpp. Enable it when all
     * installed assets tag expression and statement nodes consistently.
     */
    bool enforceChildTraits{false};

    /**
     * @brief Safety guard for while and for runtime execution.
     *
     * Set to 0 to disable the guard.
     */
    int maxLoopIterations{100000};
};

/**
 * @brief Metadata describing an installed Essentials feature.
 */
struct EssentialsFeatureInfo {
    std::string id{};
    std::string version{"0.1.0"};
    std::string description{};
    std::vector<std::string> nodeKinds{};
    std::vector<std::string> traits{};
    std::vector<std::string> requirements{};
};

/**
 * @brief Public entry point for Essentials assets.
 *
 * This controller is an asset-layer facade over EngineController.
 * It does not create a new engine, parser, runtime, AST system, or scope system.
 */
class EssentialsController {
public:
    explicit EssentialsController(
        controllers::EngineController &engine,
        EssentialsControllerOptions options = {}
    );

    EssentialsController &installScopedBlocks();
    EssentialsController &installVariables();
    EssentialsController &installExpressionStatements();
    EssentialsController &installIfStatements();
    EssentialsController &installWhileLoops();
    EssentialsController &installForLoops();
    EssentialsController &installReturnStatements();
    EssentialsController &installFunctions();

    EssentialsController &installStandardScopes();
    EssentialsController &installStandardVariables();
    EssentialsController &installStandardControlFlow();
    EssentialsController &installStandardFunctions();
    EssentialsController &installStandardCore();

    controllers::EngineController &engine();
    const controllers::EngineController &engine() const;

    const EssentialsControllerOptions &options() const;

    const std::string &statementDomain() const;
    const std::string &expressionDomain() const;

    bool hasFeature(const std::string &id) const;
    const std::vector<EssentialsFeatureInfo> &features() const;

    void registerFeature(EssentialsFeatureInfo info);

private:
    void validateOptions() const;
    void validateFeature(const EssentialsFeatureInfo &info) const;

    controllers::EngineController &engine_;
    EssentialsControllerOptions options_;
    std::vector<EssentialsFeatureInfo> features_;
    std::unordered_set<std::string> featureIds_;
};

} // namespace novac::assets::essentials
