#include "novac/assets/essentials/variables/ExpressionStatements.hpp"

#include "novac/assets/essentials/EssentialTraits.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/assets/essentials/internal/ParsingHelpers.hpp"
#include "novac/assets/essentials/internal/SchemaHelpers.hpp"

namespace novac::assets::essentials::variables {

void installExpressionStatements(EssentialsController &controller) {
    if (controller.hasFeature("essentials.expression.statement"))
        return;

    const auto &options{controller.options()};

    controller.engine().symbol(options.semicolonToken);

    controller.engine().node({
        options.expressionStatementNodeKind,
        {
            {options.expressionField, ast::FieldKind::Node, true, {}, internal::expressionTraits(controller)}
        },
        {traits::Statement},
        "Expression statement."
    });

    controller.engine().fallback(
        options.statementDomain,
        [&controller](parser::ParserContext &context) -> ast::NodePtr {
            const auto &options{controller.options()};

            if (context.check(options.rightBraceToken)) {
                return nullptr;
            }

            ast::NodePtr expression{context.parse(controller.expressionDomain())};
            internal::maybeConsumeSemicolon(context, controller);

            ast::NodePtr node{controller.engine().makeNode(options.expressionStatementNodeKind)};
            node->set(options.expressionField, expression);
            return node;
        }
    );

    controller.engine().statement(
        options.expressionStatementNodeKind,
        [&options](const ast::Node &node, runtime::RuntimeContext &context) {
            static_cast<void>(context.eval(*node.child(options.expressionField)));
        }
    );

    controller.registerFeature({"essentials.expression.statement", "0.1.0", "Expression statements",
        {options.expressionStatementNodeKind}, {traits::Statement}, {}
    });
}

} // namespace novac::assets::essentials::variables
