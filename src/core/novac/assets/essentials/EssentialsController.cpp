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

void rejectEmpty(const std::string &value, const char *name) {
    if (value.empty()) {
        throw std::runtime_error(std::string{"EssentialsControllerOptions: "} + name + " cannot be empty");
    }
}

} // namespace

EssentialsController::EssentialsController(
    controllers::EngineController &engine,
    EssentialsControllerOptions options
)
    : engine_{engine},
      options_{std::move(options)},
      features_{},
      ownedFeatures_{},
      featureIds_{} {
    validateOptions();
    engine_.setStartDomain(options_.core.programDomain);
}

EssentialsController &EssentialsController::use(const EssentialFeature &feature) {
    EssentialInfo info{feature.info()};
    validateFeature(info);
    feature.install(*this);
    rememberFeature(std::move(info));
    return *this;
}

EssentialsController &EssentialsController::use(EssentialPack pack) {
    for (auto &feature : pack.features) {
        own(std::move(feature));
    }

    pack.features.clear();
    return *this;
}

EssentialsController &EssentialsController::own(std::unique_ptr<EssentialFeature> feature) {
    if (!feature) {
        throw std::runtime_error("EssentialsController::own: feature cannot be null");
    }

    EssentialInfo info{feature->info()};
    validateFeature(info);
    feature->install(*this);
    rememberFeature(std::move(info));
    ownedFeatures_.push_back(std::move(feature));
    return *this;
}

EssentialsController &EssentialsController::installProgram() {
    const std::string id{"essentials.program"};

    if (hasFeature(id)) {
        throw std::runtime_error("EssentialsController::installProgram: duplicate feature '" + id + "'");
    }

    const CoreSyntaxOptions options{options_.core};

    engine_.node({
        options.programNodeKind,
        {
            helpers::nodeListField(
                options.statementsField,
                true,
                {},
                helpers::maybeTraits(options_.enforceChildTraits, {traits::Statement, traits::Declaration})
            )
        },
        {traits::Program},
        "Essentials program root."
    });

    engine_.fallback(options.programDomain, [options](parser::ParserContext &context) {
        ast::NodeList statements{};

        while (!context.end()) {
            statements.push_back(context.parse(options.statementDomain));
        }

        ast::NodePtr program{ast::Node::make(options.programNodeKind)};
        program->set(options.statementsField, std::move(statements));
        return program;
    });

    engine_.expression(options.programNodeKind, [options](const ast::Node &node, runtime::RuntimeContext &context) {
        const ast::NodeList &items{node.list(options.statementsField)};

        for (const ast::NodePtr &item : items) {
            if (!item) {
                throw std::runtime_error("Program: null top-level node");
            }

            if (item->kind() == "FunctionDeclaration") {
                context.bindNode(item->str("name"), item);
            }
        }

        if (ast::NodePtr main{context.boundNode("main")}) {
            ast::NodePtr call{ast::Node::make("FunctionCall")};
            call->set("name", std::string{"main"});
            call->set("arguments", ast::NodeList{});
            return context.eval(*call);
        }

        for (const ast::NodePtr &item : items) {
            context.exec(*item);

            if (context.hasReturn()) {
                return context.takeReturn();
            }
        }

        return runtime::Value::voidValue();
    });

    rememberFeature({
        id,
        "0.1.0",
        "Program root parsing and execution",
        {options.programNodeKind},
        {traits::Program},
        {}
    });

    return *this;
}

EssentialsController &EssentialsController::installScopedBlocks() {
    return use(scopes::standard());
}

EssentialsController &EssentialsController::installVariables() {
    return use(variables::variables());
}

EssentialsController &EssentialsController::installExpressionStatements() {
    return use(variables::expressionStatements());
}

EssentialsController &EssentialsController::installIfStatements() {
    return use(controlflow::ifStatements());
}

EssentialsController &EssentialsController::installWhileLoops() {
    return use(controlflow::whileLoops());
}

EssentialsController &EssentialsController::installForLoops() {
    return use(controlflow::forLoops());
}

EssentialsController &EssentialsController::installReturnStatements() {
    return use(functions::returnStatements());
}

EssentialsController &EssentialsController::installFunctions() {
    return use(functions::functions());
}

EssentialsController &EssentialsController::installStandardScopes() {
    return installScopedBlocks();
}

EssentialsController &EssentialsController::installStandardVariables() {
    use(variables::standard());
    return *this;
}

EssentialsController &EssentialsController::installStandardControlFlow() {
    use(controlflow::standard());
    return *this;
}

EssentialsController &EssentialsController::installStandardFunctions() {
    use(functions::standard());
    return *this;
}

EssentialsController &EssentialsController::installStandardCore() {
    installProgram();
    installStandardScopes();
    installStandardVariables();
    installStandardControlFlow();
    installStandardFunctions();

    engine_.setStartDomain(options_.core.programDomain);

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

const CoreSyntaxOptions &EssentialsController::core() const {
    return options_.core;
}

const VariableSyntaxOptions &EssentialsController::variables() const {
    return options_.variables;
}

const ControlFlowSyntaxOptions &EssentialsController::controlFlow() const {
    return options_.controlFlow;
}

const FunctionSyntaxOptions &EssentialsController::functions() const {
    return options_.functions;
}

bool EssentialsController::hasFeature(const std::string &id) const {
    return featureIds_.find(id) != featureIds_.end();
}

const std::vector<EssentialInfo> &EssentialsController::features() const {
    return features_;
}

void EssentialsController::validateOptions() const {
    rejectEmpty(options_.core.programDomain, "programDomain");
    rejectEmpty(options_.core.statementDomain, "statementDomain");
    rejectEmpty(options_.core.expressionDomain, "expressionDomain");
    rejectEmpty(options_.core.programNodeKind, "programNodeKind");
    rejectEmpty(options_.core.blockNodeKind, "blockNodeKind");
    rejectEmpty(options_.core.statementsField, "statementsField");
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

    rejectEmpty(options_.controlFlow.ifKeyword, "ifKeyword");
    rejectEmpty(options_.controlFlow.elseKeyword, "elseKeyword");
    rejectEmpty(options_.controlFlow.whileKeyword, "whileKeyword");
    rejectEmpty(options_.controlFlow.forKeyword, "forKeyword");

    rejectEmpty(options_.functions.functionKeyword, "functionKeyword");
    rejectEmpty(options_.functions.returnKeyword, "returnKeyword");
    rejectEmpty(options_.functions.printFunctionName, "printFunctionName");
    rejectEmpty(options_.functions.mainFunctionName, "mainFunctionName");
}

void EssentialsController::validateFeature(const EssentialInfo &info) const {
    if (info.id.empty()) {
        throw std::runtime_error("EssentialsController::validateFeature: feature id cannot be empty");
    }

    if (hasFeature(info.id)) {
        throw std::runtime_error("EssentialsController::validateFeature: duplicate feature '" + info.id + "'");
    }
}

void EssentialsController::rememberFeature(EssentialInfo info) {
    featureIds_.insert(info.id);
    features_.push_back(std::move(info));
}

namespace essentials {

EssentialPack standard() {
    EssentialPack pack{};
    pack.merge(scopes::standard());
    pack.merge(variables::standard());
    pack.merge(controlflow::standard());
    pack.merge(functions::standard());
    return pack;
}

} // namespace essentials

} // namespace novac::assets::essentials
