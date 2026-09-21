#include "novac/assets/essentials/controlflow/IfStatements.hpp"

#include "novac/assets/essentials/EssentialTraits.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/assets/essentials/helpers/SchemaHelpers.hpp"

namespace novac::assets::essentials::controlflow {

EssentialInfo IfStatementsFeature::info() const {
    return {
        "essentials.controlflow.if",
        "0.1.0",
        "If and else statements",
        {"IfStatement"},
        {traits::Statement},
        {"controlflow.if"},
        {}
    };
}

void IfStatementsFeature::install(EssentialsController &controller) const {
    const CoreSyntaxOptions core{controller.core()};
    const ControlFlowSyntaxOptions options{controller.controlFlow()};
    const bool enforce{controller.options().enforceChildTraits};

    controller.engine().keyword(options.ifKeyword);
    controller.engine().keyword(options.elseKeyword);

    controller.engine().node({
        options.ifNodeKind,
        {
            helpers::nodeField(options.conditionField, true, {}, helpers::maybeTraits(enforce, {traits::Expression})),
            helpers::nodeField(options.thenField, true, {}, helpers::maybeTraits(enforce, {traits::Statement})),
            helpers::nodeField(options.elseField, false, {}, helpers::maybeTraits(enforce, {traits::Statement}))
        },
        {traits::Statement},
        "If statement."
    });

    controller.engine().parseRule(core.statementDomain, options.ifKeyword, [core, options](parser::ParserContext &context) {
        context.consume(options.ifKeyword);
        ast::NodePtr condition{context.parse(core.expressionDomain)};
        ast::NodePtr thenBranch{context.parse(core.statementDomain)};
        ast::NodePtr elseBranch{};

        if (context.check(options.elseKeyword)) {
            context.consume(options.elseKeyword);
            elseBranch = context.parse(core.statementDomain);
        }

        ast::NodePtr node{ast::Node::make(options.ifNodeKind)};
        node->set(options.conditionField, condition);
        node->set(options.thenField, thenBranch);

        if (elseBranch) {
            node->set(options.elseField, elseBranch);
        }

        return node;
    });

    controller.engine().statement(options.ifNodeKind, [options](const ast::Node &node, runtime::RuntimeContext &context) {
        if (context.eval(*node.child(options.conditionField)).truthy()) {
            context.exec(*node.child(options.thenField));
        } else if (node.has(options.elseField)) {
            context.exec(*node.child(options.elseField));
        }
    });
}

EssentialPack ifStatements() {
    EssentialPack pack{};
    pack.add<IfStatementsFeature>();
    return pack;
}

} // namespace novac::assets::essentials::controlflow
