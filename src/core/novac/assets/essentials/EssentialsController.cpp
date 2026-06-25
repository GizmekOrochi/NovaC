#include "novac/assets/essentials/EssentialsController.hpp"

#include "novac/assets/essentials/scopes/ScopedBlocks.hpp"
#include "novac/assets/essentials/variables/ExpressionStatements.hpp"
#include "novac/assets/essentials/variables/Variables.hpp"
#include "novac/assets/essentials/controlflow/ForLoops.hpp"
#include "novac/assets/essentials/controlflow/IfStatements.hpp"
#include "novac/assets/essentials/controlflow/WhileLoops.hpp"
#include "novac/assets/essentials/functions/Functions.hpp"
#include "novac/assets/essentials/functions/ReturnStatements.hpp"

#include <stdexcept>
#include <utility>

namespace novac::assets::essentials {

EssentialsController::EssentialsController(controllers::EngineController &engine, EssentialsControllerOptions options)
    : engine_{engine}, options_{std::move(options)}, features_{}, featureIds_{} {
    validateOptions();
}

EssentialsController &EssentialsController::installScopedBlocks() {
    scopes::installScopedBlocks(*this);
    return *this;
}

EssentialsController &EssentialsController::installVariables() {
    variables::installVariables(*this);
    return *this;
}

EssentialsController &EssentialsController::installExpressionStatements() {
    variables::installExpressionStatements(*this);
    return *this;
}

EssentialsController &EssentialsController::installIfStatements() {
    controlflow::installIfStatements(*this);
    return *this;
}

EssentialsController &EssentialsController::installWhileLoops() {
    controlflow::installWhileLoops(*this);
    return *this;
}

EssentialsController &EssentialsController::installForLoops() {
    controlflow::installForLoops(*this);
    return *this;
}

EssentialsController &EssentialsController::installReturnStatements() {
    functions::installReturnStatements(*this);
    return *this;
}

EssentialsController &EssentialsController::installFunctions() {
    functions::installFunctions(*this);
    return *this;
}

EssentialsController &EssentialsController::installStandardScopes() {
    return installScopedBlocks();
}

EssentialsController &EssentialsController::installStandardVariables() {
    installVariables();
    installExpressionStatements();
    return *this;
}

EssentialsController &EssentialsController::installStandardControlFlow() {
    installIfStatements();
    installWhileLoops();
    installForLoops();
    return *this;
}

EssentialsController &EssentialsController::installStandardFunctions() {
    installReturnStatements();
    installFunctions();
    return *this;
}

EssentialsController &EssentialsController::installStandardCore() {
    installStandardScopes();
    installStandardVariables();
    installStandardControlFlow();
    installStandardFunctions();
    return *this;
}

controllers::EngineController &EssentialsController::engine() {
    return engine_;
}

const controllers::EngineController &EssentialsController::engine() const {
    return engine_;
}

const EssentialsControllerOptions &EssentialsController::options() const {
    return options_;
}

const std::string &EssentialsController::statementDomain() const {
    return options_.statementDomain;
}

const std::string &EssentialsController::expressionDomain() const {
    return options_.expressionDomain;
}

bool EssentialsController::hasFeature(const std::string &id) const {
    return featureIds_.find(id) != featureIds_.end();
}

const std::vector<EssentialsFeatureInfo> &EssentialsController::features() const {
    return features_;
}

void EssentialsController::registerFeature(EssentialsFeatureInfo info) {
    validateFeature(info);
    featureIds_.insert(info.id);
    features_.push_back(std::move(info));
}

void EssentialsController::validateOptions() const {
    const auto require = [](const std::string &value, const char *name) {
        if (value.empty())
            throw std::runtime_error(std::string{"EssentialsController: option '"} + name + "' cannot be empty");
    };

    require(options_.statementDomain, "statementDomain");
    require(options_.expressionDomain, "expressionDomain");

    require(options_.blockNodeKind, "blockNodeKind");
    require(options_.statementsField, "statementsField");

    require(options_.variableDeclarationNodeKind, "variableDeclarationNodeKind");
    require(options_.variableExpressionNodeKind, "variableExpressionNodeKind");
    require(options_.assignmentNodeKind, "assignmentNodeKind");
    require(options_.expressionStatementNodeKind, "expressionStatementNodeKind");

    require(options_.ifNodeKind, "ifNodeKind");
    require(options_.whileNodeKind, "whileNodeKind");
    require(options_.forNodeKind, "forNodeKind");

    require(options_.returnNodeKind, "returnNodeKind");
    require(options_.functionDeclarationNodeKind, "functionDeclarationNodeKind");
    require(options_.functionCallNodeKind, "functionCallNodeKind");

    require(options_.nameField, "nameField");
    require(options_.valueField, "valueField");
    require(options_.expressionField, "expressionField");
    require(options_.conditionField, "conditionField");
    require(options_.thenField, "thenField");
    require(options_.elseField, "elseField");
    require(options_.bodyField, "bodyField");
    require(options_.initializerField, "initializerField");
    require(options_.stepField, "stepField");
    require(options_.parametersField, "parametersField");
    require(options_.argumentsField, "argumentsField");

    require(options_.letKeyword, "letKeyword");
    require(options_.ifKeyword, "ifKeyword");
    require(options_.elseKeyword, "elseKeyword");
    require(options_.whileKeyword, "whileKeyword");
    require(options_.forKeyword, "forKeyword");
    require(options_.returnKeyword, "returnKeyword");
    require(options_.functionKeyword, "functionKeyword");

    require(options_.assignToken, "assignToken");
    require(options_.semicolonToken, "semicolonToken");
    require(options_.commaToken, "commaToken");
    require(options_.leftBraceToken, "leftBraceToken");
    require(options_.rightBraceToken, "rightBraceToken");
    require(options_.leftParenToken, "leftParenToken");
    require(options_.rightParenToken, "rightParenToken");
}

void EssentialsController::validateFeature(const EssentialsFeatureInfo &info) const {
    if (info.id.empty())
        throw std::runtime_error("EssentialsController::registerFeature: feature id cannot be empty");

    if (featureIds_.find(info.id) != featureIds_.end())
        throw std::runtime_error("EssentialsController::registerFeature: duplicate feature '" + info.id + "'");
}

} // namespace novac::assets::essentials
