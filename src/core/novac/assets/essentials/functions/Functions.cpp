#include "novac/assets/essentials/functions/Functions.hpp"

#include "novac/assets/essentials/EssentialTraits.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/assets/essentials/internal/ParsingHelpers.hpp"
#include "novac/assets/essentials/internal/SchemaHelpers.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace novac::assets::essentials::functions {

namespace {

ast::NodeList makeParameterNodes(EssentialsController &controller, std::vector<std::string> names) {
    const auto &options{controller.options()};

    ast::NodeList parameters{};
    parameters.reserve(names.size());

    for (std::string &name : names) {
        ast::NodePtr parameter{controller.engine().makeNode(options.parameterNodeKind)};
        parameter->set(options.nameField, std::move(name));
        parameters.push_back(std::move(parameter));
    }

    return parameters;
}

std::vector<std::string> parameterNames(const ast::NodeList &parameters, const std::string &nameField) {
    std::vector<std::string> names{};
    names.reserve(parameters.size());

    for (const ast::NodePtr &parameter : parameters) {
        if (!parameter)
            throw std::runtime_error("Functions: function declaration contains null parameter");

        names.push_back(parameter->str(nameField));
    }

    return names;
}

} // namespace

void installFunctions(EssentialsController &controller) {
    if (controller.hasFeature("essentials.functions"))
        return;

    const auto &options{controller.options()};

    controller.engine().keyword(options.functionKeyword);
    controller.engine().symbol(options.leftParenToken);
    controller.engine().symbol(options.rightParenToken);
    controller.engine().symbol(options.commaToken);

    controller.engine().node({options.parameterNodeKind, {{options.nameField, ast::FieldKind::String, true, {}, {}}}, {}, "Function parameter."});

    controller.engine().node({
        options.functionDeclarationNodeKind,
        {
            {options.nameField, ast::FieldKind::String, true, {}, {}},
            {options.parametersField, ast::FieldKind::NodeList, true, {options.parameterNodeKind}, {}},
            {options.bodyField, ast::FieldKind::Node, true, {}, internal::statementTraits(controller)}
        },
        {traits::Statement, traits::Declaration, traits::Function},
        "Function declaration."
    });

    controller.engine().node({
        options.functionCallNodeKind,
        {
            {options.nameField, ast::FieldKind::String, true, {}, {}},
            {options.argumentsField, ast::FieldKind::NodeList, true, {}, internal::expressionTraits(controller)}
        },
        {traits::Expression, traits::Function},
        "Function call expression."
    });

    controller.engine().parseRule(
        options.statementDomain,
        options.functionKeyword,
        [&controller](parser::ParserContext &context) {
            const auto &options{controller.options()};

            context.consume(options.functionKeyword);
            const token::Token &name = context.consumeKind(token::Kind::Identifier);

            std::vector<std::string> parsedParameters{internal::parseParameterList(context, controller)};
            ast::NodeList parameters{makeParameterNodes(controller, std::move(parsedParameters))};
            ast::NodePtr body{context.parse(controller.statementDomain())};

            ast::NodePtr node{controller.engine().makeNode(options.functionDeclarationNodeKind)};
            node->set(options.nameField, name.text);
            node->set(options.parametersField, std::move(parameters));
            node->set(options.bodyField, body);

            return node;
        }
    );

    controller.engine().statement(
        options.functionDeclarationNodeKind,
        [&options](const ast::Node &node, runtime::RuntimeContext &context) {
            context.bindNode(node.str(options.nameField), std::make_shared<ast::Node>(node));
        }
    );

    controller.engine().expression(
        options.functionCallNodeKind,
        [&options](const ast::Node &node, runtime::RuntimeContext &context) {
            const std::string &name{node.str(options.nameField)};
            ast::NodePtr function{context.boundNode(name)};

            if (!function) {
                throw std::runtime_error("Functions: unknown function '" + name + "'");
            }

            std::vector<std::string> parameters{parameterNames(function->list(options.parametersField), options.nameField)};
            const ast::NodeList &arguments{node.list(options.argumentsField)};

            if (parameters.size() != arguments.size()) {
                throw std::runtime_error("Functions: invalid argument count for function '" + name + "'");
            }

            std::vector<runtime::Value> values{};
            values.reserve(arguments.size());

            for (const ast::NodePtr &argument : arguments) {
                if (!argument) {
                    throw std::runtime_error("Functions: null argument in function call '" + name + "'");
                }

                values.push_back(context.eval(*argument));
            }

            context.pushScope();

            try {
                for (std::size_t i{}; i < parameters.size(); ++i) {
                    if (!context.env().define(parameters[i], std::move(values[i]))) {
                        throw std::runtime_error("Functions: duplicate parameter '" + parameters[i] + "'");
                    }
                }

                context.exec(*function->child(options.bodyField));

                runtime::Value result{context.hasReturn()
                    ? context.takeReturn()
                    : runtime::Value::voidValue()
                };

                context.popScope();

                return result;
            } catch (...) {
                context.popScope();
                throw;
            }
        }
    );

    controller.registerFeature({"essentials.functions", "0.1.0", "Function declarations and calls", {options.functionDeclarationNodeKind, options.functionCallNodeKind, options.parameterNodeKind}, {traits::Statement, traits::Expression, traits::Declaration, traits::Function}, {}});
}

} // namespace novac::assets::essentials::functions
