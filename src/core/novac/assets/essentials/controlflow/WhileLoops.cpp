#include "novac/assets/essentials/controlflow/WhileLoops.hpp"

#include "novac/assets/essentials/EssentialTraits.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/assets/essentials/helpers/SchemaHelpers.hpp"

#include <stdexcept>

namespace novac::assets::essentials::controlflow {

EssentialInfo WhileLoopsFeature::info() const {
    return {
        "essentials.controlflow.while",
        "0.1.0",
        "While loops",
        {"WhileStatement"},
        {traits::Statement},
        {}
    };
}

void WhileLoopsFeature::install(EssentialsController &controller) const {
    const CoreSyntaxOptions core{controller.core()};
    const ControlFlowSyntaxOptions options{controller.controlFlow()};
    const bool enforce{controller.options().enforceChildTraits};

    controller.engine().keyword(options.whileKeyword);

    controller.engine().node({
        options.whileNodeKind,
        {
            helpers::nodeField(options.conditionField, true, {}, helpers::maybeTraits(enforce, {traits::Expression})),
            helpers::nodeField(options.bodyField, true, {}, helpers::maybeTraits(enforce, {traits::Statement}))
        },
        {traits::Statement},
        "While loop."
    });

    controller.engine().parseRule(core.statementDomain, options.whileKeyword, [core, options](parser::ParserContext &context) {
        context.consume(options.whileKeyword);
        ast::NodePtr condition{context.parse(core.expressionDomain)};
        ast::NodePtr body{context.parse(core.statementDomain)};

        ast::NodePtr node{ast::Node::make(options.whileNodeKind)};
        node->set(options.conditionField, condition);
        node->set(options.bodyField, body);
        return node;
    });

    controller.engine().statement(options.whileNodeKind, [options](const ast::Node &node, runtime::RuntimeContext &context) {
        int iterations{};

        while (context.eval(*node.child(options.conditionField)).truthy()) {
            if (options.maxLoopIterations > 0 && iterations++ >= options.maxLoopIterations) {
                throw std::runtime_error("WhileLoopsFeature: maximum loop iteration count exceeded");
            }

            context.exec(*node.child(options.bodyField));

            if (context.hasReturn()) {
                break;
            }
        }
    });
}

EssentialPack whileLoops() {
    EssentialPack pack{};
    pack.add<WhileLoopsFeature>();
    return pack;
}


} // namespace novac::assets::essentials::controlflow
