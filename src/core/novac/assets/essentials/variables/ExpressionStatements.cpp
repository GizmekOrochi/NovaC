#include "novac/assets/essentials/variables/ExpressionStatements.hpp"

#include "novac/assets/essentials/EssentialTraits.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/assets/essentials/helpers/SchemaHelpers.hpp"

namespace novac::assets::essentials::variables {

EssentialInfo ExpressionStatementsFeature::info() const {
    return {"essentials.variables.expression-statements", "0.1.0", "Expression statements", {"ExpressionStatement"}, {traits::Statement}, {}};
}

void ExpressionStatementsFeature::install(EssentialsController &controller) const {
    const CoreSyntaxOptions options{controller.core()};
    const bool enforce{controller.options().enforceChildTraits};

    controller.engine().symbol(options.semicolonToken);

    controller.engine().node({
        options.expressionStatementNodeKind,
        {helpers::nodeField(options.expressionField, true, {}, helpers::maybeTraits(enforce, {traits::Expression}))},
        {traits::Statement},
        "Expression statement."
    });

    controller.engine().fallback(options.statementDomain, [options](parser::ParserContext &context) -> ast::NodePtr {
        ast::NodePtr expression{context.parse(options.expressionDomain)};

        if (!context.check(options.semicolonToken))
            return nullptr;

        context.consume(options.semicolonToken);

        ast::NodePtr node{ast::Node::make(options.expressionStatementNodeKind)};
        node->set(options.expressionField, expression);
        return node;
    });

    controller.engine().statement(options.expressionStatementNodeKind, [options](const ast::Node &node, runtime::RuntimeContext &context) {
        context.eval(*node.child(options.expressionField));
    });
}

EssentialPack expressionStatements() {
    EssentialPack pack{};
    pack.add<ExpressionStatementsFeature>();
    return pack;
}

} // namespace novac::assets::essentials::variables
