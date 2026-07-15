#include "novac/assets/essentials/controlflow/ForLoops.hpp"

#include "novac/assets/essentials/controlflow/IfStatements.hpp"
#include "novac/assets/essentials/controlflow/WhileLoops.hpp"
#include "novac/assets/essentials/EssentialTraits.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/assets/essentials/helpers/SchemaHelpers.hpp"

#include <stdexcept>

namespace novac::assets::essentials::controlflow {

namespace {

ast::NodePtr parseAssignmentNoSemicolon(parser::ParserContext &context, const CoreSyntaxOptions &core, const VariableSyntaxOptions &variables) {
    const token::Token name{context.consumeKind(token::Kind::Identifier)};

    context.consume(variables.assignToken);

    ast::NodePtr value{context.parse(core.expressionDomain)};

    ast::NodePtr node{ast::Node::make(variables.assignmentNodeKind)};
    node->set(variables.nameField, name.text);
    node->set(variables.valueField, value);

    return node;
}

} // namespace

EssentialInfo ForLoopsFeature::info() const {
    return {"essentials.controlflow.for", "0.1.0", "For loops", {"ForStatement"}, {traits::Statement}, {}};
}

void ForLoopsFeature::install(EssentialsController &controller) const {
    const CoreSyntaxOptions core{controller.core()};
    const VariableSyntaxOptions variables{controller.variables()};
    const ControlFlowSyntaxOptions options{controller.controlFlow()};
    const bool enforce{controller.options().enforceChildTraits};

    controller.engine().keyword(options.forKeyword);

    controller.engine().node({
        options.forNodeKind,
        {
            helpers::nodeField(options.initializerField, false, {}, helpers::maybeTraits(enforce, {traits::Statement})),
            helpers::nodeField(options.conditionField, false, {}, helpers::maybeTraits(enforce, {traits::Expression})),
            helpers::nodeField(options.stepField, false, {}, helpers::maybeTraits(enforce, {traits::Statement})),
            helpers::nodeField(options.bodyField, true, {}, helpers::maybeTraits(enforce, {traits::Statement}))
        },
        {traits::Statement},
        "For loop."
    });

    controller.engine().parseRule(core.statementDomain, options.forKeyword, [core, variables, options](parser::ParserContext &context) {
        context.consume(options.forKeyword);
        context.consume(core.leftParenToken);

        ast::NodePtr initializer{};

        if (!context.check(core.semicolonToken))
            initializer = parseAssignmentNoSemicolon(context, core, variables);

        context.consume(core.semicolonToken);

        ast::NodePtr condition{};

        if (!context.check(core.semicolonToken))
            condition = context.parse(core.expressionDomain);

        context.consume(core.semicolonToken);

        ast::NodePtr step{};

        if (!context.check(core.rightParenToken))
            step = parseAssignmentNoSemicolon(context, core, variables);

        context.consume(core.rightParenToken);

        ast::NodePtr body{context.parse(core.statementDomain)};
        ast::NodePtr node{ast::Node::make(options.forNodeKind)};

        if (initializer) node->set(options.initializerField, initializer);
        if (condition) node->set(options.conditionField, condition);
        if (step) node->set(options.stepField, step);

        node->set(options.bodyField, body);

        return node;
    });

    controller.engine().statement(options.forNodeKind, [options](const ast::Node &node, runtime::RuntimeContext &context) {
        context.pushScope();

        try {
            if (node.has(options.initializerField))
                context.exec(*node.child(options.initializerField));

            int iterations{};

            while (!node.has(options.conditionField) || context.eval(*node.child(options.conditionField)).truthy()) {
                if (options.maxLoopIterations > 0 && iterations++ >= options.maxLoopIterations)
                    throw std::runtime_error("ForLoopsFeature: maximum loop iteration count exceeded");

                context.exec(*node.child(options.bodyField));

                if (context.hasReturn())
                    break;

                if (node.has(options.stepField))
                    context.exec(*node.child(options.stepField));
            }

            context.popScope();
        } catch (...) {
            context.popScope();
            throw;
        }
    });
}

EssentialPack forLoops() {
    EssentialPack pack{};
    pack.add<ForLoopsFeature>();
    return pack;
}

EssentialPack standard() {
    EssentialPack pack{};
    pack.merge(ifStatements());
    pack.merge(whileLoops());
    pack.merge(forLoops());
    return pack;
}

} // namespace novac::assets::essentials::controlflow