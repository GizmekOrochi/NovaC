#include "../../../tester.hpp"

#include "novac/assets/essentials/helpers/SchemaHelpers.hpp"
#include "novac/engine/syntax/Node.hpp"

namespace {

using novac::ast::FieldKind;

TEST(SchemaHelpers, MaybeTraitsDisabledReturnsEmpty) {
    const auto traits{novac::assets::essentials::helpers::maybeTraits(false, {"expression", "statement"})};

    CHECK(traits.empty());
}

TEST(SchemaHelpers, MaybeTraitsEnabledReturnsTraits) {
    const auto traits{novac::assets::essentials::helpers::maybeTraits(true, {"expression", "statement"})};

    CHECK(traits.size() == 2);
    CHECK(traits[0] == "expression");
    CHECK(traits[1] == "statement");
}

TEST(SchemaHelpers, NodeFieldBuildsSchema) {
    const auto field{novac::assets::essentials::helpers::nodeField("value", false, {"IntegerLiteral"}, {"expression"})};

    CHECK(field.name == "value");
    CHECK(field.kind == FieldKind::Node);
    CHECK(!field.required);
    CHECK(field.allowedNodeKinds.size() == 1);
    CHECK(field.allowedNodeKinds[0] == "IntegerLiteral");
    CHECK(field.allowedNodeTraits.size() == 1);
    CHECK(field.allowedNodeTraits[0] == "expression");
}

TEST(SchemaHelpers, NodeListFieldBuildsSchema) {
    const auto field{novac::assets::essentials::helpers::nodeListField("items", true, {"Statement"}, {"statement"})};

    CHECK(field.name == "items");
    CHECK(field.kind == FieldKind::NodeListField);
    CHECK(field.required);
    CHECK(field.allowedNodeKinds.size() == 1);
    CHECK(field.allowedNodeKinds[0] == "Statement");
    CHECK(field.allowedNodeTraits.size() == 1);
    CHECK(field.allowedNodeTraits[0] == "statement");
}

TEST(SchemaHelpers, StringFieldBuildsSchema) {
    const auto field{novac::assets::essentials::helpers::stringField("name", true)};

    CHECK(field.name == "name");
    CHECK(field.kind == FieldKind::String);
    CHECK(field.required);
    CHECK(field.allowedNodeKinds.empty());
    CHECK(field.allowedNodeTraits.empty());
}

} // namespace
