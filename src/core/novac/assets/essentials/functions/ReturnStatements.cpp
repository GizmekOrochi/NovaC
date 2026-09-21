#include "novac/assets/essentials/functions/ReturnStatements.hpp"

#include "novac/assets/essentials/EssentialTraits.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/assets/essentials/helpers/SchemaHelpers.hpp"

namespace novac::assets::essentials::functions {

EssentialInfo ReturnStatementsFeature::info() const {
    return {"essentials.functions.return", "0.1.0", "Return statements", {"ReturnStatement"}, {traits::Statement}, {"statement.return"}, {}};
}

void ReturnStatementsFeature::install(EssentialsController &controller) const {
    const CoreSyntaxOptions core{controller.core()};
    const FunctionSyntaxOptions options{controller.functions()};
    const bool enforce{controller.options().enforceChildTraits};

    controller.engine().keyword(options.returnKeyword);
    controller.engine().symbol(core.semicolonToken);

    controller.engine().node({
        options.returnNodeKind,
        {helpers::nodeField(options.valueField, false, {}, helpers::maybeTraits(enforce, {traits::Expression}))},
        {traits::Statement},
        "Return statement."
    });

    controller.engine().parseRule(core.statementDomain, options.returnKeyword, [core, options](parser::ParserContext &context) {
        context.consume(options.returnKeyword);

        ast::NodePtr value{};

        if (!context.check(core.semicolonToken))
            value = context.parse(core.expressionDomain);

        context.consume(core.semicolonToken);

        ast::NodePtr node{ast::Node::make(options.returnNodeKind)};

        if (value)
            node->set(options.valueField, value);

        return node;
    });

    controller.engine().statement(options.returnNodeKind, [options](const ast::Node &node, runtime::RuntimeContext &context) {
        if (node.has(options.valueField)) context.returnValue(context.eval(*node.child(options.valueField)));
        else context.returnValue(runtime::Value::voidValue());
    });
}

EssentialPack returnStatements() {
    EssentialPack pack{};
    pack.add<ReturnStatementsFeature>();
    return pack;
}

} // namespace novac::assets::essentials::functions
