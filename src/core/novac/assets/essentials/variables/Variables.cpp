#include "novac/assets/essentials/variables/Variables.hpp"

#include "novac/assets/essentials/EssentialTraits.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/assets/essentials/internal/ParsingHelpers.hpp"
#include "novac/assets/essentials/internal/SchemaHelpers.hpp"

#include <stdexcept>
#include <utility>

namespace novac::assets::essentials::variables {

namespace {

ast::NodePtr parseVariableDeclaration(EssentialsController &controller, parser::ParserContext &context, bool consumeSemicolon) {
    const auto &options{controller.options()};

    context.consume(options.letKeyword);
    const token::Token &name{context.consumeKind(token::Kind::Identifier)};
    context.consume(options.assignToken);

    ast::NodePtr value{context.parse(controller.expressionDomain())};

    if (consumeSemicolon)
        internal::maybeConsumeSemicolon(context, controller);

    ast::NodePtr node{controller.engine().makeNode(options.variableDeclarationNodeKind)};
    node->set(options.nameField, name.text);
    node->set(options.valueField, value);

    return node;
}

ast::NodePtr parseAssignment(EssentialsController &controller, parser::ParserContext &context, bool consumeSemicolon) {
    const auto &options{controller.options()};

    const token::Token &name{context.consumeKind(token::Kind::Identifier)};
    context.consume(options.assignToken);

    ast::NodePtr value{context.parse(controller.expressionDomain())};

    if (consumeSemicolon)
        internal::maybeConsumeSemicolon(context, controller);

    ast::NodePtr node{controller.engine().makeNode(options.assignmentNodeKind)};
    node->set(options.nameField, name.text);
    node->set(options.valueField, value);

    return node;
}

} // namespace

void installVariables(EssentialsController &controller) {
    if (controller.hasFeature("essentials.variables"))
        return;

    const auto &options{controller.options()};

    controller.engine().keyword(options.letKeyword);
    controller.engine().symbol(options.assignToken);
    controller.engine().symbol(options.semicolonToken);

    controller.engine().node({
        options.variableDeclarationNodeKind,
        {
            {options.nameField, ast::FieldKind::String, true, {}, {}},
            {options.valueField, ast::FieldKind::Node, true, {}, internal::expressionTraits(controller)}
        },
        {traits::Statement, traits::Declaration, traits::Variable},
        "Variable declaration statement."
    });

    controller.engine().node({
        options.variableExpressionNodeKind,
        {
            {options.nameField, ast::FieldKind::String, true, {}, {}}
        },
        {traits::Expression, traits::Variable},
        "Variable reference expression."
    });

    controller.engine().node({
        options.assignmentNodeKind,
        {
            {options.nameField, ast::FieldKind::String, true, {}, {}},
            {options.valueField, ast::FieldKind::Node, true, {}, internal::expressionTraits(controller)}
        },
        {traits::Statement, traits::Variable},
        "Variable assignment statement."
    });

    controller.engine().parseRule(
        options.statementDomain,
        options.letKeyword,
        [&controller](parser::ParserContext &context) {
            return parseVariableDeclaration(controller, context, true);
        }
    );

    controller.engine().fallback(
        options.statementDomain,
        [&controller](parser::ParserContext &context) -> ast::NodePtr {
            const auto &options{controller.options()};

            if (context.cur().kind == token::Kind::Identifier && context.peek().text == options.assignToken) {
                return parseAssignment(controller, context, true);
            }

            return nullptr;
        }
    );

    controller.engine().prefix(
        options.expressionDomain,
        "$identifier",
        [&controller](parser::ParserContext &context) {
            const auto &options{controller.options()};
            const token::Token &name{context.consumeKind(token::Kind::Identifier)};

            if (context.check(options.leftParenToken)) {
                ast::NodeList arguments{internal::parseArgumentList(context, controller)};

                ast::NodePtr call{controller.engine().makeNode(options.functionCallNodeKind)};
                call->set(options.nameField, name.text);
                call->set(options.argumentsField, std::move(arguments));
                return call;
            }

            ast::NodePtr node{controller.engine().makeNode(options.variableExpressionNodeKind)};
            node->set(options.nameField, name.text);
            return node;
        }
    );

    controller.engine().statement(
        options.variableDeclarationNodeKind,
        [&options](const ast::Node &node, runtime::RuntimeContext &context) {
            const std::string &name{node.str(options.nameField)};
            runtime::Value value{context.eval(*node.child(options.valueField))};

            if (!context.env().define(name, std::move(value))) {
                throw std::runtime_error("Variables: variable '" + name + "' is already defined in this scope");
            }
        }
    );

    controller.engine().statement(
        options.assignmentNodeKind,
        [&options](const ast::Node &node, runtime::RuntimeContext &context) {
            const std::string &name{node.str(options.nameField)};
            runtime::Value value{context.eval(*node.child(options.valueField))};

            if (!context.env().assign(name, std::move(value))) {
                throw std::runtime_error("Variables: cannot assign unknown variable '" + name + "'");
            }
        }
    );

    controller.engine().expression(
        options.variableExpressionNodeKind,
        [&options](const ast::Node &node, runtime::RuntimeContext &context) {
            const std::string &name{node.str(options.nameField)};
            const runtime::Value *value{context.env().resolve(name)};

            if (!value) {
                throw std::runtime_error("Variables: unknown variable '" + name + "'");
            }

            return *value;
        }
    );

    controller.registerFeature({"essentials.variables", "0.1.0", "Variable declarations, references, and assignments",
        {options.variableDeclarationNodeKind, options.variableExpressionNodeKind, options.assignmentNodeKind},
        {traits::Statement, traits::Expression, traits::Declaration, traits::Variable},
        {}
    });
}

} // namespace novac::assets::essentials::variables
