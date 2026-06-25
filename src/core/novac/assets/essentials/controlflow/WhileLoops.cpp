#include "novac/assets/essentials/controlflow/WhileLoops.hpp"

#include "novac/assets/essentials/EssentialTraits.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/assets/essentials/internal/SchemaHelpers.hpp"

#include <stdexcept>

namespace novac::assets::essentials::controlflow {

void installWhileLoops(EssentialsController &controller) {
    if (controller.hasFeature("essentials.control.while"))
        return;

    const auto &options{controller.options()};

    controller.engine().keyword(options.whileKeyword);

    controller.engine().node({
        options.whileNodeKind,
        {
            {options.conditionField, ast::FieldKind::Node, true, {}, internal::expressionTraits(controller)},
            {options.bodyField, ast::FieldKind::Node, true, {}, internal::statementTraits(controller)}
        },
        {traits::Statement, traits::ControlFlow},
        "While loop statement."
    });

    controller.engine().parseRule(
        options.statementDomain,
        options.whileKeyword,
        [&controller](parser::ParserContext &context) {
            const auto &options{controller.options()};

            context.consume(options.whileKeyword);

            ast::NodePtr condition{context.parse(controller.expressionDomain())};
            ast::NodePtr body{context.parse(controller.statementDomain())};

            ast::NodePtr node{controller.engine().makeNode(options.whileNodeKind)};
            node->set(options.conditionField, condition);
            node->set(options.bodyField, body);
            return node;
        }
    );

    controller.engine().statement(
        options.whileNodeKind,
        [&options](const ast::Node &node, runtime::RuntimeContext &context) {
            int iterations{};

            while (context.eval(*node.child(options.conditionField)).truthy()) {
                if (options.maxLoopIterations > 0 && iterations++ >= options.maxLoopIterations)
                    throw std::runtime_error("WhileLoops: maximum loop iteration count exceeded");

                context.exec(*node.child(options.bodyField));

                if (context.hasReturn())
                    break;
            }
        }
    );

    controller.registerFeature({"essentials.control.while", "0.1.0", "While loops", {options.whileNodeKind}, {traits::Statement, traits::ControlFlow}, {}});
}

} // namespace novac::assets::essentials::controlflow
