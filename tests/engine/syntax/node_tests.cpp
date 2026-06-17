#include "../../tester.hpp"
#include "novac/engine/syntax/Node.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace {

using novac::ast::Field;
using novac::ast::FieldKind;
using novac::ast::FieldSchema;
using novac::ast::Node;
using novac::ast::NodeList;
using novac::ast::NodePtr;
using novac::ast::NodeRegistry;
using novac::ast::NodeSchema;
using novac::registry::DuplicatePolicy;
using novac::registry::RegisterStatus;

template <typename Fn>
bool throwsRuntimeError(Fn &&fn) {
    try {
        fn();
    } catch (const std::runtime_error &) {
        return true;
    } catch (...) {
        return false;
    }

    return false;
}

NodeSchema literalSchema() {
    return NodeSchema{"IntegerLiteral", {FieldSchema{"value", FieldKind::Int, true, {}, {}}}, {"Expression", "Literal"}, "Integer literal expression."};
}

NodeSchema identifierSchema() {
    return NodeSchema{"Identifier", {FieldSchema{"name", FieldKind::String, true, {}, {}}}, {"Expression"}, "Identifier expression."};
}

NodeSchema binarySchema() {
    return NodeSchema{"BinaryExpression",{
            FieldSchema{"left", FieldKind::Node, true, {}, {"Expression"}},
            FieldSchema{"op", FieldKind::String, true, {}, {}},
            FieldSchema{"right", FieldKind::Node, true, {}, {"Expression"}}
        }, {"Expression"}, "Binary expression."};
}

NodeSchema blockSchema() {
    return NodeSchema{"Block",{ FieldSchema{"statements", FieldKind::NodeList, true, {}, {"Statement"}}}, {"Statement"}, "Statement block."};
}

NodeSchema expressionListSchema() {
    return NodeSchema{"ExpressionList", { FieldSchema{"items", FieldKind::NodeList, true, {}, {"Expression"}}}, {"Expression"}, "Expression list."};
}

NodeSchema allFieldKindsSchema() {
    return NodeSchema{"AllFields", {
            FieldSchema{"any", FieldKind::Any, false, {}, {}},
            FieldSchema{"intValue", FieldKind::Int, true, {}, {}},
            FieldSchema{"floatValue", FieldKind::Float, true, {}, {}},
            FieldSchema{"boolValue", FieldKind::Bool, true, {}, {}},
            FieldSchema{"stringValue", FieldKind::String, true, {}, {}},
            FieldSchema{"nodeValue", FieldKind::Node, true, {"IntegerLiteral"}, {}},
            FieldSchema{"listValue", FieldKind::NodeList, true, {"IntegerLiteral"}, {}}
        }, {}, "Schema containing every field kind."};
}

} // namespace

TEST(Node, ConstructsWithStringKind) {
    Node node{"IntegerLiteral"};

    CHECK(node.kind() == "IntegerLiteral");
    CHECK(node.fields().empty());
}

TEST(Node, ConstructsWithTypedNodeKind) {
    novac::ids::NodeKind kind{"Identifier"};
    Node node{kind};

    CHECK(node.kind() == "Identifier");
}

TEST(Node, RejectsEmptyKind) {
    CHECK(throwsRuntimeError([]() { Node node{""}; }));
}

TEST(Node, MakesSharedNodeWithStringKind) {
    NodePtr node{Node::make("IntegerLiteral")};

    CHECK(node != nullptr);
    CHECK(node->kind() == "IntegerLiteral");
}

TEST(Node, MakesSharedNodeWithTypedNodeKind) {
    novac::ids::NodeKind kind{"BinaryExpression"};
    NodePtr node{Node::make(kind)};

    CHECK(node != nullptr);
    CHECK(node->kind() == "BinaryExpression");
}

TEST(Node, SetsAndReplacesFields) {
    Node node{"IntegerLiteral"};

    Node &returned{node.set("value", 1)};
    CHECK(&returned == &node);
    CHECK(node.has("value"));
    CHECK(node.integer("value") == 1);

    node.set("value", 42);
    CHECK(node.integer("value") == 42);
    CHECK(node.fields().size() == 1);
}

TEST(Node, SetsFieldWithTypedFieldName) {
    Node node{"Identifier"};
    novac::ids::FieldName name{"name"};

    node.set(name, std::string{"x"});

    CHECK(node.has(name));
    CHECK(node.str(name) == "x");
}

TEST(Node, RejectsEmptyFieldName) {
    Node node{"IntegerLiteral"};

    CHECK(throwsRuntimeError([&]() { node.set("", 42); }));
}

TEST(Node, ReportsMissingFields) {
    Node node{"IntegerLiteral"};

    CHECK(!node.has("value"));
    CHECK(throwsRuntimeError([&]() { static_cast<void>(node.field("value")); }));
}

TEST(Node, ReturnsRawFields) {
    Node node{"AllFields"};
    node.set("value", 42);

    const Field &field{node.field("value")};

    CHECK(std::holds_alternative<int>(field));
    CHECK(std::get<int>(field) == 42);
    CHECK(node.fields().size() == 1);
}

TEST(Node, ReadsPrimitiveFields) {
    Node node{"AllFields"};

    node.set("text", std::string{"hello"});
    node.set("integer", 7);

    CHECK(node.str("text") == "hello");
    CHECK(node.integer("integer") == 7);
}

TEST(Node, RejectsPrimitiveFieldTypeMismatches) {
    Node node{"AllFields"};

    node.set("text", std::string{"hello"});
    node.set("integer", 7);

    CHECK(throwsRuntimeError([&]() { static_cast<void>(node.integer("text")); }));
    CHECK(throwsRuntimeError([&]() { static_cast<void>(node.str("integer")); }));
}

TEST(Node, ReadsChildField) {
    Node node{"Wrapper"};
    NodePtr child{Node::make("IntegerLiteral")};

    node.set("child", child);

    CHECK(node.child("child") == child);
    CHECK(node.child("child")->kind() == "IntegerLiteral");
}

TEST(Node, RejectsChildFieldTypeMismatch) {
    Node node{"Wrapper"};

    node.set("child", 42);

    CHECK(throwsRuntimeError([&]() { static_cast<void>(node.child("child")); }));
}

TEST(Node, ReadsNodeListField) {
    Node node{"Block"};
    NodePtr first{Node::make("IntegerLiteral")};
    NodePtr second{Node::make("Identifier")};

    node.set("items", NodeList{first, second});

    const NodeList &items{node.list("items")};

    CHECK(items.size() == 2);
    CHECK(items[0] == first);
    CHECK(items[1] == second);
}

TEST(Node, RejectsNodeListFieldTypeMismatch) {
    Node node{"Block"};

    node.set("items", std::string{"not a list"});

    CHECK(throwsRuntimeError([&]() { static_cast<void>(node.list("items")); }));
}

TEST(NodeRegistry, RegistersAndFindsNodeSchemas) {
    NodeRegistry registry{};

    CHECK(registry.registerNode(literalSchema()) == RegisterStatus::Inserted);

    const NodeSchema *schema{registry.find("IntegerLiteral")};

    CHECK(schema != nullptr);
    CHECK(schema->kind == "IntegerLiteral");
    CHECK(schema->fields.size() == 1);
}

TEST(NodeRegistry, FindsSchemaWithTypedNodeKind) {
    NodeRegistry registry{};
    registry.registerNode(identifierSchema());

    novac::ids::NodeKind kind{"Identifier"};
    const NodeSchema *schema{registry.find(kind)};

    CHECK(schema != nullptr);
    CHECK(schema->kind == "Identifier");
}

TEST(NodeRegistry, ReturnsNullForUnknownSchema) {
    NodeRegistry registry{};

    CHECK(registry.find("Missing") == nullptr);
}

TEST(NodeRegistry, RejectsEmptySchemaKind) {
    NodeRegistry registry{};

    CHECK(throwsRuntimeError([&]() {
        registry.registerNode(NodeSchema{});
    }));
}

TEST(NodeRegistry, RejectsDuplicateSchemasByDefault) {
    NodeRegistry registry{};

    CHECK(registry.registerNode(literalSchema()) == RegisterStatus::Inserted);

    CHECK(throwsRuntimeError([&]() {
        registry.registerNode(literalSchema());
    }));
}

TEST(NodeRegistry, IgnoresDuplicateSchemasWhenPolicyIsIgnore) {
    NodeRegistry registry{DuplicatePolicy::Ignore};

    CHECK(registry.registerNode(literalSchema()) == RegisterStatus::Inserted);
    CHECK(registry.registerNode(literalSchema()) == RegisterStatus::Ignored);

    const NodeSchema *schema{registry.find("IntegerLiteral")};
    CHECK(schema != nullptr);
    CHECK(schema->doc == "Integer literal expression.");
}

TEST(NodeRegistry, ReplacesDuplicateSchemasWhenPolicyIsReplace) {
    NodeRegistry registry{DuplicatePolicy::Replace};

    NodeSchema first{literalSchema()};
    NodeSchema second{literalSchema()};
    second.doc = "Updated documentation.";

    CHECK(registry.registerNode(first) == RegisterStatus::Inserted);
    CHECK(registry.registerNode(second) == RegisterStatus::Replaced);

    const NodeSchema *schema{registry.find("IntegerLiteral")};
    CHECK(schema != nullptr);
    CHECK(schema->doc == "Updated documentation.");
}

TEST(NodeRegistry, ChecksTraits) {
    NodeRegistry registry{};
    registry.registerNode(literalSchema());

    CHECK(registry.hasTrait("IntegerLiteral", "Expression"));
    CHECK(registry.hasTrait("IntegerLiteral", "Literal"));
    CHECK(!registry.hasTrait("IntegerLiteral", "Statement"));
    CHECK(!registry.hasTrait("Unknown", "Expression"));
}

TEST(NodeRegistry, ValidatesSimpleNode) {
    NodeRegistry registry{};
    registry.registerNode(literalSchema());

    Node node{"IntegerLiteral"};
    node.set("value", 42);

    registry.validate(node);
}

TEST(NodeRegistry, RejectsUnknownNodeKind) {
    NodeRegistry registry{};

    Node node{"Unknown"};

    CHECK(throwsRuntimeError([&]() { registry.validate(node); }));
}

TEST(NodeRegistry, RejectsUnknownFields) {
    NodeRegistry registry{};
    registry.registerNode(literalSchema());

    Node node{"IntegerLiteral"};
    node.set("value", 42);
    node.set("extra", 1);

    CHECK(throwsRuntimeError([&]() { registry.validate(node); }));
}

TEST(NodeRegistry, RejectsMissingRequiredFields) {
    NodeRegistry registry{};
    registry.registerNode(literalSchema());

    Node node{"IntegerLiteral"};

    CHECK(throwsRuntimeError([&]() { registry.validate(node); }));
}

TEST(NodeRegistry, AcceptsMissingOptionalFields) {
    NodeRegistry registry{};
    registry.registerNode(NodeSchema{"OptionalNode", { FieldSchema{"name", FieldKind::String, false, {}, {}} }, {}, {}});

    Node node{"OptionalNode"};

    registry.validate(node);
}

TEST(NodeRegistry, AcceptsOptionalMonostateField) {
    NodeRegistry registry{};
    registry.registerNode(NodeSchema{"OptionalNode",{ FieldSchema{"name", FieldKind::String, false, {}, {}} }, {}, {}});

    Node node{"OptionalNode"};
    node.set("name", std::monostate{});

    registry.validate(node);
}

TEST(NodeRegistry, RejectsRequiredMonostateField) {
    NodeRegistry registry{};
    registry.registerNode(NodeSchema{"RequiredNode",{ FieldSchema{"name", FieldKind::String, true, {}, {}} }, {}, {}});

    Node node{"RequiredNode"};
    node.set("name", std::monostate{});

    CHECK(throwsRuntimeError([&]() {registry.validate(node);}));
}

TEST(NodeRegistry, ValidatesAllSupportedFieldKinds) {
    NodeRegistry registry{};
    registry.registerNode(literalSchema());
    registry.registerNode(allFieldKindsSchema());

    NodePtr child{Node::make("IntegerLiteral")};
    child->set("value", 1);

    Node node{"AllFields"};
    node.set("any", std::string{"anything"});
    node.set("intValue", 1);
    node.set("floatValue", 2.5);
    node.set("boolValue", true);
    node.set("stringValue", std::string{"hello"});
    node.set("nodeValue", child);
    node.set("listValue", NodeList{child});

    registry.validate(node);
}

TEST(NodeRegistry, RejectsInvalidFieldTypes) {
    NodeRegistry registry{};
    registry.registerNode(literalSchema());

    Node node{"IntegerLiteral"};
    node.set("value", std::string{"not an int"});

    CHECK(throwsRuntimeError([&]() { registry.validate(node); }));
}

TEST(NodeRegistry, RejectsNullChildField) {
    NodeRegistry registry{};
    registry.registerNode(binarySchema());

    NodePtr right{Node::make("IntegerLiteral")};
    right->set("value", 2);

    Node node{"BinaryExpression"};
    node.set("left", NodePtr{});
    node.set("op", std::string{"+"});
    node.set("right", right);

    CHECK(throwsRuntimeError([&]() { registry.validate(node); }));
}

TEST(NodeRegistry, RejectsNullChildInNodeList) {
    NodeRegistry registry{};
    registry.registerNode(expressionListSchema());

    Node node{"ExpressionList"};
    node.set("items", NodeList{NodePtr{}});

    CHECK(throwsRuntimeError([&]() { registry.validate(node); }));
}

TEST(NodeRegistry, ValidatesChildConstraintsByTrait) {
    NodeRegistry registry{};
    registry.registerNode(literalSchema());
    registry.registerNode(identifierSchema());
    registry.registerNode(binarySchema());

    NodePtr left{Node::make("IntegerLiteral")};
    left->set("value", 1);

    NodePtr right{Node::make("Identifier")};
    right->set("name", std::string{"x"});

    Node node{"BinaryExpression"};
    node.set("left", left);
    node.set("op", std::string{"+"});
    node.set("right", right);

    registry.validate(node);
}

TEST(NodeRegistry, RejectsChildThatDoesNotMatchRequiredTrait) {
    NodeRegistry registry{};
    registry.registerNode(literalSchema());
    registry.registerNode(blockSchema());
    registry.registerNode(binarySchema());

    NodePtr left{Node::make("Block")};
    left->set("statements", NodeList{});

    NodePtr right{Node::make("IntegerLiteral")};
    right->set("value", 1);

    Node node{"BinaryExpression"};
    node.set("left", left);
    node.set("op", std::string{"+"});
    node.set("right", right);

    CHECK(throwsRuntimeError([&]() { registry.validate(node); }));
}

TEST(NodeRegistry, ValidatesChildConstraintsByExplicitKind) {
    NodeRegistry registry{};
    registry.registerNode(literalSchema());
    registry.registerNode(NodeSchema{"LiteralWrapper", { FieldSchema{"value", FieldKind::Node, true, {"IntegerLiteral"}, {}} }, {}, {} });

    NodePtr child{Node::make("IntegerLiteral")};
    child->set("value", 42);

    Node node{"LiteralWrapper"};
    node.set("value", child);

    registry.validate(node);
}

TEST(NodeRegistry, RejectsChildThatDoesNotMatchRequiredKind) {
    NodeRegistry registry{};
    registry.registerNode(literalSchema());
    registry.registerNode(identifierSchema());
    registry.registerNode(NodeSchema{"LiteralWrapper", { FieldSchema{"value", FieldKind::Node, true, {"IntegerLiteral"}, {}} }, {}, {} });

    NodePtr child{Node::make("Identifier")};
    child->set("name", std::string{"x"});

    Node node{"LiteralWrapper"};
    node.set("value", child);

    CHECK(throwsRuntimeError([&]() { registry.validate(node); }));
}

TEST(NodeRegistry, ValidatesNodeListChildren) {
    NodeRegistry registry{};
    registry.registerNode(literalSchema());
    registry.registerNode(identifierSchema());
    registry.registerNode(expressionListSchema());

    NodePtr first{Node::make("IntegerLiteral")};
    first->set("value", 1);

    NodePtr second{Node::make("Identifier")};
    second->set("name", std::string{"x"});

    Node node{"ExpressionList"};
    node.set("items", NodeList{first, second});

    registry.validate(node);
}

TEST(NodeRegistry, RejectsInvalidNodeListChildren) {
    NodeRegistry registry{};
    registry.registerNode(literalSchema());
    registry.registerNode(blockSchema());
    registry.registerNode(expressionListSchema());

    NodePtr block{Node::make("Block")};
    block->set("statements", NodeList{});

    Node node{"ExpressionList"};
    node.set("items", NodeList{block});

    CHECK(throwsRuntimeError([&]() { registry.validate(node); }));
}

TEST(NodeRegistry, RecursivelyValidatesChildren) {
    NodeRegistry registry{};
    registry.registerNode(literalSchema());
    registry.registerNode(binarySchema());

    NodePtr left{Node::make("IntegerLiteral")};
    left->set("value", 1);

    NodePtr right{Node::make("IntegerLiteral")};
    //Missing required "value" field on purpose.

    Node node{"BinaryExpression"};
    node.set("left", left);
    node.set("op", std::string{"+"});
    node.set("right", right);

    CHECK(throwsRuntimeError([&]() {
        registry.validate(node);
    }));
}
