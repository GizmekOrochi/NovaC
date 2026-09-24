#include <NovaC.hpp>
#include "novac/assets/atomic/LiteralFeature.hpp"

#include <array>
#include <iostream>
#include <optional>
#include <string>

using namespace novac;
using namespace novac::assets::types;

namespace {

class CardinalDirectionRepresentation final : public PrimitiveRepresentation {};

class CardinalDirectionLiteral final : public assets::atomic::LiteralFeature {
public:
    assets::atomic::LiteralInfo info() const override {
        return {
            "example.literal.cardinal-direction",
            "0.1.0",
            "Cardinal direction arrow literal",
            "CardinalDirectionLiteral",
            assets::atomic::TokenPattern::text("←"),
            {"literal.cardinal-direction", "expression.atom"},
            {}
        };
    }

    void install(assets::atomic::AtomicController &controller) const override {
        struct DirectionToken {
            const char *token;
            int value;
        };

        static constexpr std::array<DirectionToken, 4> directions{{
            {"←", 0},
            {"↑", 1},
            {"→", 2},
            {"↓", 3},
        }};

        for (const auto &direction : directions)
            controller.registerPattern(assets::atomic::TokenPattern::text(direction.token));

        controller.engine().node({
            .kind = "CardinalDirectionLiteral",
            .fields = {
                {.name = "value", .kind = ast::FieldKind::Int, .required = true}
            },
            .traits = {"expr", "literal"},
            .doc = "Two-bit cardinal direction literal"
        });

        auto *const engine{&controller.engine()};
        const std::string domain{controller.expressionDomain()};

        for (const auto &direction : directions) {
            const std::string token{direction.token};
            const int value{direction.value};

            controller.engine().prefix(domain, token, [engine, token, value](parser::ParserContext &context) {
                context.consume(token);
                ast::NodePtr node{engine->makeNode("CardinalDirectionLiteral")};
                node->set("value", value);
                return node;
            });
        }

        controller.engine().expression(
            "CardinalDirectionLiteral",
            [](const ast::Node &node, runtime::RuntimeContext &) {
                return runtime::Value::integer(node.integer("value"));
            });
    }
};

} // namespace

int main() {
    controllers::EngineController engine;
    assets::atomic::AtomicController atomic{engine};
    CardinalDirectionLiteral arrows;
    atomic.use(arrows);

    TypeController types;
    types.definePrimitive("CardinalDirection")
        .bits(2)
        .storageBits(2)
        .alignment(1)
        .unsignedType()
        .representation<CardinalDirectionRepresentation>()
        .commit();

    types.bindLiteral(arrows, TypeId{"CardinalDirection"});
    types.finalize();

    const PrimitiveType &directionType{
        types.requirePrimitive(TypeId{"CardinalDirection"})
    };

    std::cout << directionType.id.name
              << " uses " << directionType.bitWidth << " bits\n";

    for (const std::string source : {"←", "↑", "→", "↓"}) {
        const ast::NodePtr node{engine.parse(source)};
        const std::optional<TypeId> type{types.resolveLiteral(arrows, *node)};

        if (!type) {
            std::cerr << "cannot type literal " << source << '\n';
            return 1;
        }

        std::cout << source << " -> " << type->name
                  << " (" << engine.eval(*node).asInt() << ")\n";
    }
}
