#include "novac/assets/essentials/variables/Variables.hpp"

#include "novac/assets/essentials/variables/ExpressionStatements.hpp"
#include "novac/assets/essentials/EssentialTraits.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/assets/essentials/helpers/ParsingHelpers.hpp"
#include "novac/assets/essentials/helpers/SchemaHelpers.hpp"

#include <stdexcept>

namespace novac::assets::essentials::variables {

EssentialInfo VariablesFeature::info() const {
    return {"essentials.variables", "0.1.0", "Variable declarations, assignments, and lookups", {"VariableDeclaration", "VariableExpression", "AssignmentStatement"}, {traits::Expression, traits::Statement}, {}};
}

void VariablesFeature::install(EssentialsController &controller) const {
    const CoreSyntaxOptions core{controller.core()};
    const VariableSyntaxOptions options{controller.variables()};
    const bool enforce{controller.options().enforceChildTraits};

    controller.engine().keyword(options.letKeyword);
    controller.engine().symbol(options.assignToken);
    controller.engine().symbol(core.semicolonToken);

    controller.engine().node({
        options.declarationNodeKind,
        {helpers::stringField(options.nameField, true), helpers::nodeField(options.valueField, true, {},helpers::maybeTraits(enforce, {traits::Expression}))},
        {traits::Statement, traits::Declaration},
        "Variable declaration."
    });

    controller.engine().node({
        options.expressionNodeKind,
        {helpers::stringField(options.nameField, true)},
        {traits::Expression},
        "Variable lookup expression."
    });

    controller.engine().node({
        options.assignmentNodeKind,
        {helpers::stringField(options.nameField, true), helpers::nodeField(options.valueField, true, {}, helpers::maybeTraits(enforce, {traits::Expression}))},
        {traits::Statement},
        "Variable assignment statement."
    });

    controller.engine().parseRule(core.statementDomain, options.letKeyword, [core, options](parser::ParserContext &context) {
        context.consume(options.letKeyword);

        const std::string name{helpers::consumeIdentifier(context, "VariablesFeature::declaration")};

        context.consume(options.assignToken);
        ast::NodePtr value{context.parse(core.expressionDomain)};
        context.consume(core.semicolonToken);

        ast::NodePtr node{ast::Node::make(options.declarationNodeKind)};
        node->set(options.nameField, name);
        node->set(options.valueField, value);
        return node;
    });

    controller.engine().fallback(core.statementDomain, [core, options](parser::ParserContext &context) -> ast::NodePtr {
        if (context.cur().kind != token::Kind::Identifier || context.peek().text != options.assignToken)
            return nullptr;

        const std::string name{context.advance().text};
        context.consume(options.assignToken);
        ast::NodePtr value{context.parse(core.expressionDomain)};
        context.consume(core.semicolonToken);

        ast::NodePtr node{ast::Node::make(options.assignmentNodeKind)};
        node->set(options.nameField, name);
        node->set(options.valueField, value);
        return node;
    });

    controller.engine().prefix(core.expressionDomain, "$identifier", [core, options](parser::ParserContext &context) {
        const std::string name{helpers::consumeIdentifier(context, "VariablesFeature::expression")};

        if (context.check(core.leftParenToken)) {
            ast::NodeList arguments{helpers::parseExpressionList(context, core.expressionDomain, core.leftParenToken, core.rightParenToken, core.commaToken)};
            ast::NodePtr call{ast::Node::make("FunctionCall")};
            call->set("name", name);
            call->set("arguments", std::move(arguments));
            return call;
        }

        ast::NodePtr node{ast::Node::make(options.expressionNodeKind)};
        node->set(options.nameField, name);
        return node;
    });

    controller.engine().statement(options.declarationNodeKind, [options](const ast::Node &node, runtime::RuntimeContext &context) {
        const std::string name{node.str(options.nameField)};
        runtime::Value value{context.eval(*node.child(options.valueField))};

        if (!context.env().define(name, std::move(value)))
            throw std::runtime_error("VariablesFeature: duplicate variable '" + name + "'");
    });

    controller.engine().statement(options.assignmentNodeKind, [options](const ast::Node &node, runtime::RuntimeContext &context) {
        const std::string name{node.str(options.nameField)};
        runtime::Value value{context.eval(*node.child(options.valueField))};

        if (!context.env().assign(name, value))
            context.env().define(name, std::move(value));
    });

    controller.engine().expression(options.expressionNodeKind, [options](const ast::Node &node, runtime::RuntimeContext &context) {
        const std::string name{node.str(options.nameField)};
        const runtime::Value *value{context.env().resolve(name)};

        if (!value) {
            throw std::runtime_error("VariablesFeature: unknown variable '" + name + "'");
        }

        return *value;
    });
}

EssentialPack variables() {
    EssentialPack pack{};
    pack.add<VariablesFeature>();
    return pack;
}

EssentialPack standard() {
    EssentialPack pack{};
    pack.merge(variables());
    pack.merge(expressionStatements());
    return pack;
}

} // namespace novac::assets::essentials::variables
