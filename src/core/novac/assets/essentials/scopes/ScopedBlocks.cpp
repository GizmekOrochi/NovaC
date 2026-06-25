#include "novac/assets/essentials/scopes/ScopedBlocks.hpp"

#include "novac/assets/essentials/EssentialTraits.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/assets/essentials/internal/ParsingHelpers.hpp"
#include "novac/assets/essentials/internal/SchemaHelpers.hpp"

#include <stdexcept>

namespace novac::assets::essentials::scopes {

void installScopedBlocks(EssentialsController &controller) {
    if (controller.hasFeature("essentials.scope.block"))
        return;

    const auto &options{controller.options()};

    controller.engine().symbol(options.leftBraceToken);
    controller.engine().symbol(options.rightBraceToken);

    controller.engine().node({
        options.blockNodeKind,
        {
            {
                options.statementsField,
                ast::FieldKind::NodeList,
                true,
                {},
                internal::statementTraits(controller)
            }
        },
        {traits::Statement, traits::Scope},
        "Lexical scoped block statement."
    });

    controller.engine().parseRule(
        options.statementDomain,
        options.leftBraceToken,
        [&controller](parser::ParserContext &context) {
            const auto &options{controller.options()};

            context.consume(options.leftBraceToken);

            ast::NodeList statements{};

            while (!context.check(options.rightBraceToken)) {
                if (context.end()) {
                    throw std::runtime_error("ScopedBlocks: unterminated block statement");
                }

                statements.push_back(context.parse(controller.statementDomain()));
            }

            context.consume(options.rightBraceToken);

            ast::NodePtr node{controller.engine().makeNode(options.blockNodeKind)};
            node->set(options.statementsField, std::move(statements));

            return node;
        }
    );

    controller.engine().statement(
        options.blockNodeKind,
        [&options](const ast::Node &node, runtime::RuntimeContext &context) {
            context.pushScope();

            try {
                for (const ast::NodePtr &statement : node.list(options.statementsField)) {
                    if (!statement) {
                        throw std::runtime_error("ScopedBlocks: block contains null statement");
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
        }
    );

    controller.registerFeature({"essentials.scope.block", "0.1.0", "Lexical scoped block statement",
        {options.blockNodeKind}, {traits::Statement, traits::Scope}, {}
    });
}

} // namespace novac::assets::essentials::scopes
