#include "novac/assets/essentials/controlflow/IfStatements.hpp"

#include "novac/assets/essentials/EssentialTraits.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/assets/essentials/internal/SchemaHelpers.hpp"

#include <variant>

namespace novac::assets::essentials::controlflow {

void installIfStatements(EssentialsController &controller) {
    if (controller.hasFeature("essentials.control.if"))
        return;

    const auto &options{controller.options()};

    controller.engine().keyword(options.ifKeyword);
    controller.engine().keyword(options.elseKeyword);

    controller.engine().node({
        options.ifNodeKind,
        {
            {options.conditionField, ast::FieldKind::Node, true, {}, internal::expressionTraits(controller)},
            {options.thenField, ast::FieldKind::Node, true, {}, internal::statementTraits(controller)},
            {options.elseField, ast::FieldKind::Node, false, {}, internal::statementTraits(controller)}
        },
        {traits::Statement, traits::ControlFlow},
        "If/else statement."
    });

    controller.engine().parseRule(
        options.statementDomain,
        options.ifKeyword,
        [&controller](parser::ParserContext &context) {
            const auto &options{controller.options()};

            context.consume(options.ifKeyword);

            ast::NodePtr condition{context.parse(controller.expressionDomain())};
            ast::NodePtr thenBranch{context.parse(controller.statementDomain())};

            ast::NodePtr elseBranch{};

            if (context.check(options.elseKeyword)) {
                context.advance();
                elseBranch = context.parse(controller.statementDomain());
            }

            ast::NodePtr node{controller.engine().makeNode(options.ifNodeKind)};
            node->set(options.conditionField, condition);
            node->set(options.thenField, thenBranch);

            if (elseBranch) node->set(options.elseField, elseBranch);
            else node->set(options.elseField, std::monostate{});

            return node;
        }
    );

    controller.engine().statement(
        options.ifNodeKind,
        [&options](const ast::Node &node, runtime::RuntimeContext &context) {
            if (context.eval(*node.child(options.conditionField)).truthy()) {
                context.exec(*node.child(options.thenField));
                return;
            }

            if (node.has(options.elseField) && !std::holds_alternative<std::monostate>(node.field(options.elseField)))
                context.exec(*node.child(options.elseField));
        }
    );

    controller.registerFeature({"essentials.control.if", "0.1.0", "If/else statements", {options.ifNodeKind}, {traits::Statement, traits::ControlFlow}, {}});
}

} // namespace novac::assets::essentials::controlflow
