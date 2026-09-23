#pragma once

#include "../foundation/Ids.hpp"
#include "../foundation/registry/Registry.hpp"
#include "../foundation/registry/RegistryHelpers.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace novac::ast {

class Node;

/**
 * @brief Shared pointer used to reference AST nodes.
 *
 * Nodes can be stored inside other nodes while remaining shareable by other
 * parts of the language pipeline.
 */
using NodePtr = std::shared_ptr<Node>;

/**
 * @brief Ordered collection of child AST nodes.
 */
using NodeList = std::vector<NodePtr>;

/**
 * @brief Generic value stored inside an AST node field.
 *
 * Fields can contain primitive values, a single child node, a list of child
 * nodes, or an empty value represented by std::monostate.
 */
using Field = std::variant<std::monostate, int, double, bool, std::string, NodePtr, NodeList>;

/**
 * @brief Supported schema field types.
 *
 * A FieldKind describes which alternative of Field is accepted by a schema.
 * FieldKind::Any disables type restriction for the field.
 */
enum class FieldKind {
    Any,
    Int,
    Float,
    Bool,
    String,
    Node,
    NodeListField
};

/**
 * @brief Describes a field accepted by a node schema.
 *
 * A field schema defines the expected field name and type, whether the field
 * is required, and optional restrictions on child node kinds or traits.
 */
struct FieldSchema {
    /** Name used to store the field inside the node. */
    std::string name{};

    /** Expected value type for the field. */
    FieldKind kind{FieldKind::Any};

    /** Whether validation requires this field to be present. */
    bool required{true};

    /** Node kinds accepted when the field contains child nodes. */
    std::vector<std::string> allowedNodeKinds{};

    /**
     * @brief Traits accepted for child nodes.
     *
     * If this list is not empty, a child is accepted when its registered
     * schema provides at least one of these traits.
     */
    std::vector<std::string> allowedNodeTraits{};
};

/**
 * @brief Describes a node kind and its validation rules.
 *
 * Schemas are registered in NodeRegistry and define the expected structure of
 * AST nodes created by language features.
 */
struct NodeSchema {
    /** Node kind handled by this schema. */
    std::string kind{};

    /** Fields that can appear on nodes of this kind. */
    std::vector<FieldSchema> fields{};

    /**
     * @brief Traits provided by this node kind.
     *
     * Traits allow schemas to accept groups of compatible node kinds without
     * listing every concrete kind individually.
     */
    std::vector<std::string> traits{};

    /** Optional human-readable documentation for the node kind. */
    std::string doc{};
};

/**
 * @brief Generic abstract syntax tree node.
 *
 * A node is identified by a string kind and stores its data in named fields.
 * This keeps the AST independent from a fixed language grammar: language
 * features can define new node kinds without modifying the Node class.
 *
 * Fields may contain primitive values, child nodes or lists of child nodes.
 * Their expected structure can optionally be checked through NodeRegistry.
 */
class Node {
public:
    /**
     * @brief Creates a node of the specified kind.
     *
     * The node initially contains no fields. They can be added later with
     * set().
     *
     * @param kind Node kind identifier.
     *
     * @throws std::runtime_error If the kind is empty.
     */
    explicit Node(std::string kind);

    /**
     * @brief Creates a node using a strongly typed node kind.
     *
     * This overload forwards the identifier value to the string-based
     * constructor.
     *
     * @param kind Node kind identifier.
     */
    explicit Node(const ids::NodeKind &kind);

    /**
     * @brief Creates a shared node instance.
     *
     * This is a convenience helper around std::make_shared<Node>().
     *
     * @param kind Node kind identifier.
     * @return Shared ownership of the created node.
     */
    static NodePtr make(std::string kind);

    /**
     * @brief Creates a shared node instance using a typed node kind.
     *
     * @param kind Node kind identifier.
     * @return Shared ownership of the created node.
     */
    static NodePtr make(const ids::NodeKind &kind);

    /**
     * @brief Returns the node kind.
     *
     * @return Node kind identifier.
     */
    const std::string &kind() const;

    /**
     * @brief Assigns or replaces a field value.
     *
     * If a field with the same name already exists, its previous value is
     * replaced. The node itself is returned to allow chained set() calls.
     *
     * @param name Field name.
     * @param value Field value.
     * @return Reference to this node.
     *
     * @throws std::runtime_error If the field name is empty.
     */
    Node &set(std::string name, Field value);

    /**
     * @brief Assigns or replaces a field using a typed field name.
     *
     * @param name Field name.
     * @param value Field value.
     * @return Reference to this node.
     */
    Node &set(const ids::FieldName &name, Field value);

    /**
     * @brief Checks whether a field exists.
     *
     * @param name Field name.
     * @return True if the field is present.
     */
    bool has(const std::string &name) const;

    /**
     * @brief Checks whether a typed field name exists.
     *
     * @param name Field name.
     * @return True if the field is present.
     */
    bool has(const ids::FieldName &name) const;

    /**
     * @brief Returns a field value.
     *
     * The returned value keeps its generic Field representation. Typed helpers
     * such as str(), integer(), child() and list() can be used when the
     * expected type is known.
     *
     * @param name Field name.
     * @return Field value.
     *
     * @throws std::runtime_error If the field does not exist.
     */
    const Field &field(const std::string &name) const;

    /**
     * @brief Returns a field using a typed field name.
     *
     * @param name Field name.
     * @return Field value.
     */
    const Field &field(const ids::FieldName &name) const;

    /**
     * @brief Returns all fields stored in the node.
     *
     * This is mainly useful for generic traversal, validation and lowering
     * code that does not know every field name in advance.
     *
     * @return Field map.
     */
    const std::unordered_map<std::string, Field> &fields() const;

    /**
     * @brief Returns a field as a string.
     *
     * The function first retrieves the named field and then verifies that its
     * stored variant type is std::string.
     *
     * @param name Field name.
     * @return Stored string value.
     *
     * @throws std::runtime_error If the field is missing or has a different type.
     */
    const std::string &str(const std::string &name) const;

    /**
     * @brief Returns a typed field name as a string value.
     */
    const std::string &str(const ids::FieldName &name) const;

    /**
     * @brief Returns a field as an integer.
     *
     * The function first retrieves the named field and then verifies that its
     * stored variant type is int.
     *
     * @param name Field name.
     * @return Stored integer value.
     *
     * @throws std::runtime_error If the field is missing or has a different type.
     */
    int integer(const std::string &name) const;

    /**
     * @brief Returns a typed field name as an integer value.
     */
    int integer(const ids::FieldName &name) const;

    /**
     * @brief Returns a child node field.
     *
     * The field must contain a NodePtr. Schema validation can additionally
     * restrict which child node kinds are accepted.
     *
     * @param name Field name.
     * @return Child node reference.
     *
     * @throws std::runtime_error If the field is missing or has a different type.
     */
    NodePtr child(const std::string &name) const;

    /**
     * @brief Returns a child node using a typed field name.
     */
    NodePtr child(const ids::FieldName &name) const;

    /**
     * @brief Returns a child node list field.
     *
     * @param name Field name.
     * @return Child node list.
     *
     * @throws std::runtime_error If the field is missing or has a different type.
     */
    const NodeList &list(const std::string &name) const;

    /**
     * @brief Returns a child node list using a typed field name.
     */
    const NodeList &list(const ids::FieldName &name) const;

private:
    std::string kind_;
    std::unordered_map<std::string, Field> fields_;
};

/**
 * @brief Stores node schemas and validates AST structures.
 *
 * NodeRegistry defines the structural rules for generic AST nodes. Language
 * features register schemas describing their own node kinds, expected fields
 * and allowed child relationships.
 *
 * Validation is recursive: after the current node is checked, every reachable
 * child node and node list is validated using its own registered schema.
 */
class NodeRegistry {
public:
    /**
     * @brief Creates a node registry.
     *
     * @param duplicatePolicy Policy used when duplicate schemas are registered.
     */
    explicit NodeRegistry(registry::DuplicatePolicy duplicatePolicy = registry::DuplicatePolicy::Error);

    /**
     * @brief Registers a node schema.
     *
     * The schema is stored by its node kind and later used by find(),
     * hasTrait() and validate().
     *
     * @param schema Schema to register.
     * @return Registration result.
     *
     * @throws std::runtime_error If the schema kind is empty.
     */
    registry::RegisterStatus registerNode(NodeSchema schema);

    /**
     * @brief Finds a schema by node kind.
     *
     * @param kind Node kind identifier.
     * @return Schema pointer, or nullptr if not found.
     */
    const NodeSchema *find(const std::string &kind) const;

    /**
     * @brief Finds a schema using a typed node kind identifier.
     *
     * @param kind Node kind identifier.
     * @return Schema pointer, or nullptr if not found.
     */
    const NodeSchema *find(const ids::NodeKind &kind) const;

    /**
     * @brief Checks whether a node kind provides a trait.
     *
     * The registered schema is searched and its trait list is inspected. An
     * unknown node kind simply returns false.
     *
     * @param nodeKind Node kind identifier.
     * @param trait Trait name.
     * @return True if the trait is present.
     */
    bool hasTrait(const std::string &nodeKind, const std::string &trait) const;

    /**
     * @brief Validates a node and all reachable child nodes.
     *
     * Validation first finds the schema for the node kind, rejects unknown
     * fields, checks required fields and field types, then recursively validates
     * every child node.
     *
     * Child fields can also be restricted by allowed node kinds or traits.
     *
     * @param node Root node to validate.
     *
     * @throws std::runtime_error If validation fails.
     */
    void validate(const Node &node) const;

private:
    /**
     * @brief Checks whether a generic field matches a schema field type.
     */
    static bool fieldMatchesKind(const Field &field, FieldKind kind);

    /**
     * @brief Rejects fields that are not declared by the node schema.
     */
    void validateFieldKnown(const NodeSchema &schema, const std::string &fieldName, const Node &node) const;

    /**
     * @brief Ensures every required schema field exists on the node.
     */
    void validateRequiredFields(const NodeSchema &schema, const Node &node) const;

    /**
     * @brief Validates stored field types and child restrictions.
     *
     * Node and NodeList fields are also checked for null children before their
     * kind and trait constraints are evaluated.
     */
    void validateFieldTypes(const NodeSchema &schema, const Node &node) const;

    /**
     * @brief Checks whether a child node is accepted by a field schema.
     *
     * Kind restrictions require an exact allowed kind. Trait restrictions
     * accept the child when at least one required trait is provided.
     */
    void validateChildConstraints(const FieldSchema &fieldSchema, const Node &child, const Node &owner) const;

    /**
     * @brief Recursively validates all child nodes reachable from a node.
     *
     * Both single NodePtr fields and NodeList fields are traversed.
     */
    void validateChildren(const Node &node) const;

    /**
     * @brief Finds the schema entry describing one field.
     *
     * @return Field schema pointer, or nullptr when the field is not declared.
     */
    const FieldSchema *findFieldSchema(const NodeSchema &schema, const std::string &fieldName) const;

    std::unordered_map<std::string, NodeSchema> schemas_;
    registry::DuplicatePolicy duplicatePolicy_;
};

} // namespace novac::ast