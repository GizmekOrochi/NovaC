#include "novac/assets/essentials/scopes/ScopedBlocks.hpp"

#include "novac/assets/essentials/EssentialTraits.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/assets/essentials/helpers/SchemaHelpers.hpp"

#include <stdexcept>

namespace novac::assets::essentials::scopes {

EssentialInfo ScopedBlocksFeature::info() const {
    return {
        "essentials.scopes.blocks",
        "0.1.0",
        "Lexical scoped blocks",
        {"BlockStmt"},
        {traits::Statement, traits::Scope},
        {}
    };
}

void ScopedBlocksFeature::install(EssentialsController &controller) const {
    const CoreSyntaxOptions options{controller.core()};
    const bool enforce{controller.options().enforceChildTraits};

    controller.engine().symbol(options.leftBraceToken);
    controller.engine().symbol(options.rightBraceToken);

    controller.engine().node({
        options.blockNodeKind,
        {
            helpers::nodeListField(
                options.statementsField,
                true,
                {},
                helpers::maybeTraits(enforce, {traits::Statement})
            )
        },
        {traits::Statement, traits::Scope},
        "Lexical scoped block."
    });

    controller.engine().parseRule(options.statementDomain, options.leftBraceToken, [options](parser::ParserContext &context) {
        context.consume(options.leftBraceToken);

        ast::NodeList statements{};

        while (!context.check(options.rightBraceToken)) {
            if (context.end()) {
                throw std::runtime_error("ScopedBlocksFeature: unterminated block");
            }

            statements.push_back(context.parse(options.statementDomain));
        }

        context.consume(options.rightBraceToken);

        ast::NodePtr block{ast::Node::make(options.blockNodeKind)};
        block->set(options.statementsField, std::move(statements));
        return block;
    });

    controller.engine().statement(options.blockNodeKind, [options](const ast::Node &node, runtime::RuntimeContext &context) {
        context.pushScope();

        try {
            for (const ast::NodePtr &statement : node.list(options.statementsField)) {
                if (!statement) {
                    throw std::runtime_error("ScopedBlocksFeature: null statement");
                }

                context.exec(*statement);

                if (context.hasReturn()) {
                    break;
                }
            }

            context.popScope();
        } catch (...) {
            context.popScope();
            throw;
        }
    });
}

EssentialPack scopedBlocks() {
    EssentialPack pack{};
    pack.add<ScopedBlocksFeature>();
    return pack;
}

EssentialPack standard() {
    return scopedBlocks();
}

} // namespace novac::assets::essentials::scopes
