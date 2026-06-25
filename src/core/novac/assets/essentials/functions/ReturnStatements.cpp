#include "novac/assets/essentials/functions/ReturnStatements.hpp"

#include "novac/assets/essentials/EssentialTraits.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/assets/essentials/internal/ParsingHelpers.hpp"
#include "novac/assets/essentials/internal/SchemaHelpers.hpp"

#include <variant>

namespace novac::assets::essentials::functions {

void installReturnStatements(EssentialsController &controller) {
    if (controller.hasFeature("essentials.function.return"))
        return;

    const auto &options{controller.options()};

    controller.engine().keyword(options.returnKeyword);
    controller.engine().symbol(options.semicolonToken);

    controller.engine().node({
        options.returnNodeKind,
        {
            {options.valueField, ast::FieldKind::Node, false, {}, internal::expressionTraits(controller)}
        },
        {traits::Statement},
        "Return statement."
    });

    controller.engine().parseRule(
        options.statementDomain,
        options.returnKeyword,
        [&controller](parser::ParserContext &context) {
            const auto &options{controller.options()};

            context.consume(options.returnKeyword);

            ast::NodePtr value{};
            if (!context.check(options.semicolonToken) && !context.check(options.rightBraceToken)) {
                value = context.parse(controller.expressionDomain());
            }

            internal::maybeConsumeSemicolon(context, controller);

            ast::NodePtr node{controller.engine().makeNode(options.returnNodeKind)};
            node->set(options.valueField, value ? ast::Field{value} : ast::Field{std::monostate{}});
            return node;
        }
    );

    controller.engine().statement(
        options.returnNodeKind,
        [&options](const ast::Node &node, runtime::RuntimeContext &context) {
            if (node.has(options.valueField) && !std::holds_alternative<std::monostate>(node.field(options.valueField))) {
                context.returnValue(context.eval(*node.child(options.valueField)));
                return;
            }

            context.returnValue(runtime::Value::voidValue());
        }
    );

    controller.registerFeature({"essentials.function.return", "0.1.0", "Return statements", {options.returnNodeKind}, {traits::Statement}, {}});
}

} // namespace novac::assets::essentials::functions
