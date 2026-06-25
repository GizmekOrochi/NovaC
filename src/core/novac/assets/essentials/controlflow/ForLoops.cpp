#include "novac/assets/essentials/controlflow/ForLoops.hpp"

#include "novac/assets/essentials/EssentialTraits.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/assets/essentials/internal/ParsingHelpers.hpp"
#include "novac/assets/essentials/internal/SchemaHelpers.hpp"

#include <stdexcept>
#include <variant>

namespace novac::assets::essentials::controlflow {

namespace {

ast::NodePtr parseOptionalStatementUntilSemicolon(EssentialsController &controller, parser::ParserContext &context) {
    const auto &options{controller.options()};

    if (context.check(options.semicolonToken)) {
        context.advance();
        return nullptr;
    }

    ast::NodePtr statement{context.parse(controller.statementDomain())};

    if (!context.check(options.semicolonToken))
        return statement;

    context.advance();
    return statement;
}

} // namespace

void installForLoops(EssentialsController &controller) {
    if (controller.hasFeature("essentials.control.for"))
        return;

    const auto &options{controller.options()};

    controller.engine().keyword(options.forKeyword);
    controller.engine().symbol(options.leftParenToken);
    controller.engine().symbol(options.rightParenToken);
    controller.engine().symbol(options.semicolonToken);

    controller.engine().node({
        options.forNodeKind,
        {
            {options.initializerField, ast::FieldKind::Node, false, {}, internal::statementTraits(controller)},
            {options.conditionField, ast::FieldKind::Node, false, {}, internal::expressionTraits(controller)},
            {options.stepField, ast::FieldKind::Node, false, {}, internal::statementTraits(controller)},
            {options.bodyField, ast::FieldKind::Node, true, {}, internal::statementTraits(controller)}
        },
        {traits::Statement, traits::ControlFlow},
        "For loop statement."
    });

    controller.engine().parseRule(
        options.statementDomain,
        options.forKeyword,
        [&controller](parser::ParserContext &context) {
            const auto &options{controller.options()};

            context.consume(options.forKeyword);
            context.consume(options.leftParenToken);

            ast::NodePtr initializer{parseOptionalStatementUntilSemicolon(controller, context)};

            ast::NodePtr condition{};
            if (!context.check(options.semicolonToken))
                condition = context.parse(controller.expressionDomain());

            context.consume(options.semicolonToken);

            ast::NodePtr step{};
            if (!context.check(options.rightParenToken))
                step = context.parse(controller.statementDomain());

            context.consume(options.rightParenToken);

            ast::NodePtr body{context.parse(controller.statementDomain())};

            ast::NodePtr node{controller.engine().makeNode(options.forNodeKind)};
            node->set(options.initializerField, initializer ? ast::Field{initializer} : ast::Field{std::monostate{}});
            node->set(options.conditionField, condition ? ast::Field{condition} : ast::Field{std::monostate{}});
            node->set(options.stepField, step ? ast::Field{step} : ast::Field{std::monostate{}});
            node->set(options.bodyField, body);

            return node;
        }
    );

    controller.engine().statement(
        options.forNodeKind,
        [&options](const ast::Node &node, runtime::RuntimeContext &context) {
            context.pushScope();

            try {
                if (node.has(options.initializerField) && !std::holds_alternative<std::monostate>(node.field(options.initializerField)))
                    context.exec(*node.child(options.initializerField));

                int iterations{};

                while (true) {
                    if (node.has(options.conditionField) && !std::holds_alternative<std::monostate>(node.field(options.conditionField)))
                        if (!context.eval(*node.child(options.conditionField)).truthy())
                            break;

                    if (options.maxLoopIterations > 0 && iterations++ >= options.maxLoopIterations)
                        throw std::runtime_error("ForLoops: maximum loop iteration count exceeded");

                    context.exec(*node.child(options.bodyField));

                    if (context.hasReturn())
                        break;

                    if (node.has(options.stepField) && !std::holds_alternative<std::monostate>(node.field(options.stepField)))
                        context.exec(*node.child(options.stepField));
                }

                context.popScope();
            } catch (...) {
                context.popScope();
                throw;
            }
        }
    );

    controller.registerFeature({"essentials.control.for", "0.1.0", "For loops", {options.forNodeKind}, {traits::Statement, traits::ControlFlow}, {}});
}

} // namespace novac::assets::essentials::controlflow
