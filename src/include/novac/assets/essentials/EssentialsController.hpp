#pragma once

#include "novac/assets/essentials/EssentialFeature.hpp"
#include "novac/assets/essentials/functions/FunctionRegistry.hpp"
#include "novac/engine/EngineController.hpp"

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace novac::assets::essentials {

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

struct VariableSyntaxOptions {
    std::string declarationNodeKind{"VariableDeclaration"};
    std::string expressionNodeKind{"VariableExpression"};
    std::string assignmentNodeKind{"AssignmentStatement"};

    std::string letKeyword{"let"};
    std::string assignToken{"="};

    std::string nameField{"name"};
    std::string valueField{"value"};
};

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

struct EssentialsControllerOptions {
    CoreSyntaxOptions core{};
    VariableSyntaxOptions variables{};
    ControlFlowSyntaxOptions controlFlow{};
    FunctionSyntaxOptions functions{};

    bool enforceChildTraits{false};
};

class EssentialsController {
public:
    explicit EssentialsController(
        controllers::EngineController &engine,
        EssentialsControllerOptions options = {}
    );

    EssentialsController &use(const EssentialFeature &feature);
    EssentialsController &use(EssentialPack pack);
    EssentialsController &own(std::unique_ptr<EssentialFeature> feature);

    EssentialsController &installProgram();
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
    const CoreSyntaxOptions &core() const;
    const VariableSyntaxOptions &variables() const;
    const ControlFlowSyntaxOptions &controlFlow() const;
    const FunctionSyntaxOptions &functions() const;

    functions::FunctionRegistry &functionRegistry();
    const functions::FunctionRegistry &functionRegistry() const;

    bool hasFeature(const std::string &id) const;
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

EssentialPack standard();

} // namespace essentials

} // namespace novac::assets::essentials
