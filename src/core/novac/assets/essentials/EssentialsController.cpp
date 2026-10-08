#include "novac/assets/essentials/EssentialsController.hpp"

#include "novac/assets/essentials/controlflow/ForLoops.hpp"
#include "novac/assets/essentials/controlflow/IfStatements.hpp"
#include "novac/assets/essentials/controlflow/WhileLoops.hpp"
#include "novac/assets/essentials/functions/Functions.hpp"
#include "novac/assets/essentials/functions/ReturnStatements.hpp"
#include "novac/assets/essentials/scopes/ScopedBlocks.hpp"
#include "novac/assets/essentials/variables/ExpressionStatements.hpp"
#include "novac/assets/essentials/variables/Variables.hpp"

#include "novac/assets/essentials/EssentialTraits.hpp"
#include "novac/assets/essentials/helpers/SchemaHelpers.hpp"

#include <stdexcept>
#include <utility>


namespace novac::assets::essentials {


namespace {

/**
 * @brief Implements the `rejectEmpty` operation.
 *
 * @param value Value supplied for `value`.
 * @param name Value supplied for `name`.
 */
void rejectEmpty(const std::string &value, const char *name) {
    if (value.empty())
        throw std::runtime_error(std::string{"EssentialsControllerOptions: "} + name + " cannot be empty");
}

}

/**
 * @brief Constructs a `EssentialsController` instance.
 *
 * @param engine Value supplied for `engine`.
 * @param options Value supplied for `options`.
 */
EssentialsController::EssentialsController(controllers::EngineController &engine, EssentialsControllerOptions options) :
    functionRegistry_{std::make_shared<functions::FunctionRegistry>()},
    engine_{engine},
    options_{std::move(options)},
    features_{},
    ownedFeatures_{},
    featureIds_{} {
    validateOptions();
    engine_.setStartDomain(options_.core.programDomain);
}

/**
 * @brief Adds the supplied behavior through `use`.
 *
 * @param feature Value supplied for `feature`.
 * @return Value produced by the operation.
 */
EssentialsController &EssentialsController::use(const EssentialFeature &feature) {
    EssentialInfo info{feature.info()};
    validateFeature(info);

    const controllers::EngineController engineSnapshot{engine_.snapshot()};
    const functions::FunctionRegistry functionRegistrySnapshot{*functionRegistry_};
    const auto featuresSnapshot{features_};
    const auto featureIdsSnapshot{featureIds_};
    const std::size_t ownedFeaturesSize{ownedFeatures_.size()};

    try {
        feature.install(*this);
        rememberFeature(std::move(info));
    } catch (...) {
        engine_.restore(engineSnapshot);
        *functionRegistry_ = functionRegistrySnapshot;
        features_ = featuresSnapshot;
        featureIds_ = featureIdsSnapshot;
        ownedFeatures_.resize(ownedFeaturesSize);
        throw;
    }

    return *this;
}

/**
 * @brief Adds the supplied behavior through `use`.
 *
 * @param pack Value supplied for `pack`.
 * @return Value produced by the operation.
 */
EssentialsController &EssentialsController::use(EssentialPack pack) {
    for(auto &feature : pack.features)
        own(std::move(feature));

    pack.features.clear();

    return *this;
}

/**
 * @brief Adds the supplied behavior through `own`.
 *
 * @param feature Value supplied for `feature`.
 * @return Value produced by the operation.
 */
EssentialsController &EssentialsController::own(std::unique_ptr<EssentialFeature> feature) {
    if(!feature)
        throw std::runtime_error("EssentialsController::own: feature cannot be null");

    use(*feature);
    ownedFeatures_.push_back(std::move(feature));

    return *this;
}

/**
 * @brief Installs the behavior provided by `installProgram`.
 *
 * @return Value produced by the operation.
 */
EssentialsController &EssentialsController::installProgram(){
    const std::string id{"essentials.program"};

    if(hasFeature(id))
        throw std::runtime_error("EssentialsController::installProgram: duplicate feature '" + id + "'");

    const CoreSyntaxOptions options{options_.core};
    const FunctionSyntaxOptions functionOptions{options_.functions};

    engine_.node({
        options.programNodeKind,
        {helpers::nodeListField(options.statementsField, true, {}, helpers::maybeTraits(options_.enforceChildTraits, {traits::Statement, traits::Declaration}))},
        {traits::Program},
        "Essentials program root."
    });

    engine_.fallback(options.programDomain,[options](parser::ParserContext &context){
            ast::NodeList statements{};

            while(!context.end()) {
                statements.push_back(context.parse(options.statementDomain));
            }

            ast::NodePtr program{ast::Node::make(options.programNodeKind)};

            program->set(options.statementsField, std::move(statements));

            return program;
        }
    );

    engine_.expression(options.programNodeKind,[options, functionOptions](const ast::Node &node, runtime::RuntimeContext &context) {
            const ast::NodeList &items{node.list(options.statementsField)};

            for(const ast::NodePtr &item : items) {
                if(!item)
                    throw std::runtime_error("Program: null top-level node");

                if(item->kind()==functionOptions.declarationNodeKind)
                    context.bindNode(item->str(functionOptions.nameField), item);
            }



            if(ast::NodePtr main = context.boundNode(functionOptions.mainFunctionName)) {
                ast::NodePtr call{ast::Node::make(functionOptions.callNodeKind)};

                call->set(functionOptions.nameField, functionOptions.mainFunctionName);
                call->set(functionOptions.argumentsField, ast::NodeList{});

                return context.eval(*call);
            }

            for(const ast::NodePtr &item : items) {
                context.exec(*item);
                if(context.hasReturn())
                    return context.takeReturn();
                if(context.hasSignal())
                    break;
            }

            return runtime::Value::voidValue();

        }
    );

    rememberFeature({id, "0.1.0", "Program root parsing and execution", {options.programNodeKind}, {traits::Program}, {"program"}, {}});

    return *this;
}



/**
 * @brief Installs the behavior provided by `installScopedBlocks`.
 *
 * @return Value produced by the operation.
 */
EssentialsController &EssentialsController::installScopedBlocks() {
    return use(scopes::standard());
}

/**
 * @brief Installs the behavior provided by `installVariables`.
 *
 * @return Value produced by the operation.
 */
EssentialsController &EssentialsController::installVariables() {
    return use(variables::variables());
}

/**
 * @brief Installs the behavior provided by `installExpressionStatements`.
 *
 * @return Value produced by the operation.
 */
EssentialsController &EssentialsController::installExpressionStatements() {
    return use(variables::expressionStatements());
}

/**
 * @brief Installs the behavior provided by `installIfStatements`.
 *
 * @return Value produced by the operation.
 */
EssentialsController &EssentialsController::installIfStatements() {
    return use(controlflow::ifStatements());
}

/**
 * @brief Installs the behavior provided by `installWhileLoops`.
 *
 * @return Value produced by the operation.
 */
EssentialsController &EssentialsController::installWhileLoops() {
    return use(controlflow::whileLoops());
}

/**
 * @brief Installs the behavior provided by `installForLoops`.
 *
 * @return Value produced by the operation.
 */
EssentialsController &EssentialsController::installForLoops() {
    return use(controlflow::forLoops());
}

/**
 * @brief Installs the behavior provided by `installReturnStatements`.
 *
 * @return Value produced by the operation.
 */
EssentialsController &EssentialsController::installReturnStatements() {
    return use(functions::returnStatements());
}

/**
 * @brief Installs the behavior provided by `installFunctions`.
 *
 * @return Value produced by the operation.
 */
EssentialsController &EssentialsController::installFunctions() {
    return use(functions::functions());
}

/**
 * @brief Installs the behavior provided by `installStandardScopes`.
 *
 * @return Value produced by the operation.
 */
EssentialsController &EssentialsController::installStandardScopes() {
    return installScopedBlocks();
}

/**
 * @brief Installs the behavior provided by `installStandardVariables`.
 *
 * @return Value produced by the operation.
 */
EssentialsController &EssentialsController::installStandardVariables() {
    use(variables::standard());
    return *this;
}

/**
 * @brief Installs the behavior provided by `installStandardControlFlow`.
 *
 * @return Value produced by the operation.
 */
EssentialsController &EssentialsController::installStandardControlFlow() {
    use(controlflow::standard());
    return *this;
}

/**
 * @brief Installs the behavior provided by `installStandardFunctions`.
 *
 * @return Value produced by the operation.
 */
EssentialsController &EssentialsController::installStandardFunctions() {
    use(functions::standard());
    return *this;
}

/**
 * @brief Installs the behavior provided by `installStandardCore`.
 *
 * @return Value produced by the operation.
 */
EssentialsController &EssentialsController::installStandardCore() {
    installProgram();
    installStandardScopes();
    installStandardVariables();
    installStandardControlFlow();
    installStandardFunctions();

    engine_.setStartDomain(options_.core.programDomain);

    return *this;
}

/**
 * @brief Returns the value exposed by `engine`.
 *
 * @return Value produced by the operation.
 */
controllers::EngineController &EssentialsController::engine() {
    return engine_;
}

/**
 * @brief Returns the value exposed by `engine`.
 *
 * @return Value produced by the operation.
 */
const controllers::EngineController &EssentialsController::engine() const {
    return engine_;
}

/**
 * @brief Implements the `functionRegistry` operation.
 *
 * @return Value produced by the operation.
 */
functions::FunctionRegistry &EssentialsController::functionRegistry() {
    return *functionRegistry_;
}

/**
 * @brief Implements the `functionRegistry` operation.
 *
 * @return Value produced by the operation.
 */
const functions::FunctionRegistry &EssentialsController::functionRegistry() const {
    return *functionRegistry_;
}

/**
 * @brief Implements the `functionRegistryHandle` operation.
 *
 * @return Value produced by the operation.
 */
std::shared_ptr<functions::FunctionRegistry> EssentialsController::functionRegistryHandle() const {
    return functionRegistry_;
}

/**
 * @brief Implements the `options` operation.
 *
 * @return Value produced by the operation.
 */
const EssentialsControllerOptions &EssentialsController::options() const {
    return options_;
}

/**
 * @brief Implements the `core` operation.
 *
 * @return Value produced by the operation.
 */
const CoreSyntaxOptions &EssentialsController::core() const {
    return options_.core;
}

/**
 * @brief Implements the `variables` operation.
 *
 * @return Value produced by the operation.
 */
const VariableSyntaxOptions &EssentialsController::variables() const {
    return options_.variables;
}

/**
 * @brief Implements the `controlFlow` operation.
 *
 * @return Value produced by the operation.
 */
const ControlFlowSyntaxOptions &EssentialsController::controlFlow() const {
    return options_.controlFlow;
}

/**
 * @brief Implements the `functions` operation.
 *
 * @return Value produced by the operation.
 */
const FunctionSyntaxOptions &EssentialsController::functions() const{
    return options_.functions;
}

/**
 * @brief Checks the condition represented by `hasFeature`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
bool EssentialsController::hasFeature(const std::string &id) const {
    return featureIds_.find(id) != featureIds_.end();
}

/**
 * @brief Checks the condition represented by `hasCapability`.
 *
 * @param capability Value supplied for `capability`.
 * @return Value produced by the operation.
 */
bool EssentialsController::hasCapability(const std::string &capability) const {
    return engine_.hasCapability(capability);
}

/**
 * @brief Implements the `features` operation.
 *
 * @return Value produced by the operation.
 */
const std::vector<EssentialInfo> &EssentialsController::features() const{
    return features_;
}

/**
 * @brief Validates data through `validateOptions`.
 */
void EssentialsController::validateOptions() const {

    rejectEmpty(options_.core.programDomain, "programDomain");
    rejectEmpty(options_.core.statementDomain, "statementDomain");
    rejectEmpty(options_.core.expressionDomain, "expressionDomain");

    rejectEmpty(options_.core.programNodeKind, "programNodeKind");
    rejectEmpty(options_.core.blockNodeKind, "blockNodeKind");
    rejectEmpty(options_.core.expressionStatementNodeKind, "expressionStatementNodeKind");

    rejectEmpty(options_.core.statementsField, "statementsField");
    rejectEmpty(options_.core.expressionField, "expressionField");

    rejectEmpty(options_.core.semicolonToken, "semicolonToken");
    rejectEmpty(options_.core.commaToken, "commaToken");
    rejectEmpty(options_.core.leftBraceToken, "leftBraceToken");
    rejectEmpty(options_.core.rightBraceToken, "rightBraceToken");
    rejectEmpty(options_.core.leftParenToken, "leftParenToken");
    rejectEmpty(options_.core.rightParenToken, "rightParenToken");

    rejectEmpty(options_.variables.declarationNodeKind, "variable declaration node kind");
    rejectEmpty(options_.variables.expressionNodeKind, "variable expression node kind");
    rejectEmpty(options_.variables.assignmentNodeKind, "assignment node kind");
    rejectEmpty(options_.variables.letKeyword, "letKeyword");
    rejectEmpty(options_.variables.assignToken, "assignToken");
    rejectEmpty(options_.variables.nameField, "variable nameField");
    rejectEmpty(options_.variables.valueField, "variable valueField");

    rejectEmpty(options_.controlFlow.ifNodeKind, "ifNodeKind");
    rejectEmpty(options_.controlFlow.whileNodeKind, "whileNodeKind");
    rejectEmpty(options_.controlFlow.forNodeKind, "forNodeKind");
    rejectEmpty(options_.controlFlow.ifKeyword, "ifKeyword");
    rejectEmpty(options_.controlFlow.elseKeyword, "elseKeyword");
    rejectEmpty(options_.controlFlow.whileKeyword, "whileKeyword");
    rejectEmpty(options_.controlFlow.forKeyword, "forKeyword");
    rejectEmpty(options_.controlFlow.conditionField, "conditionField");
    rejectEmpty(options_.controlFlow.thenField, "thenField");
    rejectEmpty(options_.controlFlow.elseField, "elseField");
    rejectEmpty(options_.controlFlow.bodyField, "bodyField");
    rejectEmpty(options_.controlFlow.initializerField, "initializerField");
    rejectEmpty(options_.controlFlow.stepField, "stepField");

    rejectEmpty(options_.functions.declarationNodeKind, "function declaration node kind");
    rejectEmpty(options_.functions.callNodeKind, "function call node kind");
    rejectEmpty(options_.functions.parameterNodeKind, "function parameter node kind");
    rejectEmpty(options_.functions.returnNodeKind, "return node kind");
    rejectEmpty(options_.functions.functionKeyword, "functionKeyword");
    rejectEmpty(options_.functions.returnKeyword, "returnKeyword");
    rejectEmpty(options_.functions.mainFunctionName, "mainFunctionName");
    rejectEmpty(options_.functions.nameField, "function nameField");
    rejectEmpty(options_.functions.bodyField, "function bodyField");
    rejectEmpty(options_.functions.parametersField, "function parametersField");
    rejectEmpty(options_.functions.argumentsField, "function argumentsField");
    rejectEmpty(options_.functions.valueField, "function valueField");

}
/**
 * @brief Validates data through `validateFeature`.
 *
 * @param info Value supplied for `info`.
 */
void EssentialsController::validateFeature(const EssentialInfo &info) const{
    if(info.id.empty())
        throw std::runtime_error("EssentialsController::validateFeature: feature id cannot be empty");

    if(hasFeature(info.id))
        throw std::runtime_error("EssentialsController::validateFeature: duplicate feature '" + info.id + "'");

    for (const std::string &capability : info.capabilities) {
        if (capability.empty())
            throw std::runtime_error("EssentialsController::validateFeature: provided capability cannot be empty");
    }

    for (const std::string &requirement : info.requirements) {
        if (requirement.empty())
            throw std::runtime_error("EssentialsController::validateFeature: required capability cannot be empty");

        if (!hasCapability(requirement))
            throw std::runtime_error("EssentialsController::validateFeature: missing required capability '" + requirement + "' for feature '" + info.id + "'");
    }

}

/**
 * @brief Implements the `rememberFeature` operation.
 *
 * @param info Value supplied for `info`.
 */
void EssentialsController::rememberFeature(EssentialInfo info){
    featureIds_.insert(info.id);
    for (const std::string &capability : info.capabilities) {
        engine_.registerCapability(capability);
    }
    features_.push_back(std::move(info));
}

namespace essentials {

/**
 * @brief Configures the standard behavior provided by `standard`.
 *
 * @return Value produced by the operation.
 */
EssentialPack standard() {
    EssentialPack pack{};

    pack.merge(scopes::standard());
    pack.merge(variables::standard());
    pack.merge(controlflow::standard());
    pack.merge(functions::standard());

    return pack;
}

}

}
