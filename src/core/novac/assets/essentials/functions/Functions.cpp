#include "novac/assets/essentials/functions/Functions.hpp"


#include "novac/assets/essentials/functions/FunctionMain.hpp"
#include "novac/assets/essentials/functions/ReturnStatements.hpp"

#include "novac/assets/essentials/EssentialTraits.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"

#include "novac/assets/essentials/helpers/ParsingHelpers.hpp"
#include "novac/assets/essentials/helpers/SchemaHelpers.hpp"


#include <stdexcept>


namespace novac::assets::essentials::functions {


EssentialInfo FunctionsFeature::info() const {
    return {"essentials.functions", "0.1.0", "Function declarations and calls", {"FunctionDeclaration", "FunctionCall", "FunctionParameter"}, {traits::Declaration, traits::Expression, traits::Callable}, {"functions"}, {}};
}


void FunctionsFeature::install(EssentialsController &controller) const {
    const CoreSyntaxOptions core{controller.core()};
    const FunctionSyntaxOptions options{controller.functions()};
    const bool enforce{controller.options().enforceChildTraits};

    controller.engine().keyword(options.functionKeyword);

    controller.engine().symbol(core.leftParenToken);
    controller.engine().symbol(core.rightParenToken);
    controller.engine().symbol(core.commaToken);

    controller.engine().node({options.parameterNodeKind, {helpers::stringField(options.nameField, true)}, {}, "Function parameter."});
    controller.engine().node({
        options.declarationNodeKind,
        {helpers::stringField(options.nameField, true), helpers::nodeListField(options.parametersField, true, {options.parameterNodeKind}, {}), helpers::nodeField(options.bodyField, true, {}, helpers::maybeTraits(enforce, {traits::Statement}))},
        {traits::Declaration, traits::Statement, traits::Callable},
        "Function declaration."
    });

    controller.engine().node({
        options.callNodeKind,
        {helpers::stringField(options.nameField, true), helpers::nodeListField(options.argumentsField, true, {}, helpers::maybeTraits(enforce, {traits::Expression}))},
        {traits::Expression},
        "Function call expression."
    });

    controller.engine().parseRule(core.statementDomain, options.functionKeyword,[core, options](parser::ParserContext &context) {
            context.consume(options.functionKeyword);

            const std::string name{helpers::consumeIdentifier(context, "FunctionsFeature::declaration")};

            std::vector<std::string> parameterNames{helpers::parseIdentifierList(context, core.leftParenToken, core.rightParenToken, core.commaToken)};

            ast::NodeList parameters{};

            for (const std::string &parameterName : parameterNames) {
                ast::NodePtr parameter{ast::Node::make(options.parameterNodeKind)};
                parameter->set(options.nameField, parameterName);
                parameters.push_back(parameter);
            }

            ast::NodePtr body{context.parse(core.statementDomain)};
            ast::NodePtr node{ast::Node::make(options.declarationNodeKind)};

            node->set(options.nameField, name);
            node->set(options.parametersField, std::move(parameters));
            node->set(options.bodyField, body);

            return node;
        }
    );

    controller.engine().statement(options.declarationNodeKind,[options](const ast::Node &node, runtime::RuntimeContext &context) {
            ast::NodePtr function{ast::Node::make(options.declarationNodeKind)};

            function->set(options.nameField, node.str(options.nameField));
            function->set(options.parametersField, node.list(options.parametersField));
            function->set(options.bodyField, node.child(options.bodyField));

            context.bindNode(node.str(options.nameField), function);
        }
    );

    controller.engine().expression(
        options.callNodeKind,[options, &controller](const ast::Node &node, runtime::RuntimeContext &context) {
            const std::string name{node.str(options.nameField)};
            const ast::NodeList &arguments{node.list(options.argumentsField)};

            if(controller.functionRegistry().hasNative(name))
                return controller.functionRegistry().getNative(name)( arguments, context);

            ast::NodePtr function{context.boundNode(name)};

            if(!function)
                throw std::runtime_error("FunctionsFeature: unknown function '" + name + "'");

            const ast::NodeList &parameters{function->list(options.parametersField)};

            if(parameters.size() != arguments.size())
                throw std::runtime_error("FunctionsFeature: wrong argument count for function '" + name + "'");

            std::vector<runtime::Value> argumentValues{};
            argumentValues.reserve(arguments.size());

            for(const ast::NodePtr &argument : arguments)
                argumentValues.push_back(context.eval(*argument));

            context.pushScope();

            try {
                for(std::size_t index{}; index < parameters.size(); ++index) {
                    const std::string parameterName{parameters[index]->str(options.nameField)};
                    context.env().define(parameterName, std::move(argumentValues[index]));
                }

                context.exec(*function->child(options.bodyField));

                runtime::Value result{runtime::Value::voidValue()};

                if(context.hasReturn())
                    result = context.takeReturn();

                context.popScope();

                return result;

            }
            catch(...) {
                context.popScope();
                throw;
            }

        }
    );
}

EssentialPack functions() {
    EssentialPack pack{};
    pack.add<FunctionsFeature>();

    return pack;
}

EssentialPack standard() {
    EssentialPack pack{};

    pack.merge(returnStatements());
    pack.merge(functions());
    pack.merge(functionMain());

    return pack;
}

} // namespace novac::assets::essentials::functions