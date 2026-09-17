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

using NodePtr = std::shared_ptr<Node>;
using NodeList = std::vector<NodePtr>;
using Field = std::variant<std::monostate, int, double, bool, std::string, NodePtr, NodeList>;

/**
 * @brief Supported schema field types.
 */
enum class FieldKind {
    Any,
    Int,
    Float,
    Bool,
    String,
    Node,
    NodeList
};

/**
 * @brief Describes a field accepted by a node schema.
 */
struct FieldSchema {
    std::string name{};
    FieldKind kind{FieldKind::Any};
    bool required{true};
    std::vector<std::string> allowedNodeKinds{};
    std::vector<std::string> allowedNodeTraits{};
};

/**
 * @brief Describes a node kind and its validation rules.
 */
struct NodeSchema {
    std::string kind{};
    std::vector<FieldSchema> fields{};
    std::vector<std::string> traits{};
    std::string doc{};
};

/**
 * @brief Generic abstract syntax tree node.
 *
 * Fields are stored as named variant values and may contain primitive
 * values, child nodes, or child node lists.
 */
class Node {
public:
    /**
     * @brief Creates a node of the specified kind.
     *
     * @param kind Node kind identifier.
     *
     * @throws std::runtime_error If the kind is empty.
     */
    explicit Node(std::string kind);

    /**
     * @brief Creates a node using a strongly typed node kind.
     *
     * @param kind Node kind identifier.
     */
    explicit Node(const ids::NodeKind &kind);

    /**
     * @brief Creates a shared node instance.
     *
     * @param kind Node kind identifier.
     * @return Shared ownership of the created node.
     */
    static NodePtr make(std::string kind);

    /**
     * @brief Creates a shared node instance.
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
     * @param name Field name.
     * @param value Field value.
     * @return Reference to this node.
     *
     * @throws std::runtime_error If the field name is empty.
     */
    Node &set(std::string name, Field value);

    /**
     * @brief Assigns or replaces a field value.
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
     * @brief Checks whether a field exists.
     *
     * @param name Field name.
     * @return True if the field is present.
     */
    bool has(const ids::FieldName &name) const;

    /**
     * @brief Returns a field value.
     *
     * @param name Field name.
     * @return Field value.
     *
     * @throws std::runtime_error If the field does not exist.
     */
    const Field &field(const std::string &name) const;

    /**
     * @brief Returns a field value.
     *
     * @param name Field name.
     * @return Field value.
     */
    const Field &field(const ids::FieldName &name) const;

    /**
     * @brief Returns all fields stored in the node.
     *
     * @return Field map.
     */
    const std::unordered_map<std::string, Field> &fields() const;

    /**
     * @brief Returns a field as a string.
     *
     * @param name Field name.
     * @return Stored string value.
     *
     * @throws std::runtime_error If the field is missing or has a different type.
     */
    const std::string &str(const std::string &name) const;

    const std::string &str(const ids::FieldName &name) const;

    /**
     * @brief Returns a field as an integer.
     *
     * @param name Field name.
     * @return Stored integer value.
     *
     * @throws std::runtime_error If the field is missing or has a different type.
     */
    int integer(const std::string &name) const;

    int integer(const ids::FieldName &name) const;

    /**
     * @brief Returns a child node field.
     *
     * @param name Field name.
     * @return Child node reference.
     *
     * @throws std::runtime_error If the field is missing or has a different type.
     */
    NodePtr child(const std::string &name) const;

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

    const NodeList &list(const ids::FieldName &name) const;

private:
    std::string kind_;
    std::unordered_map<std::string, Field> fields_;
};

/**
 * @brief Stores node schemas and validates AST structures.
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
     * @brief Finds a schema by node kind.
     *
     * @param kind Node kind identifier.
     * @return Schema pointer, or nullptr if not found.
     */
    const NodeSchema *find(const ids::NodeKind &kind) const;

    /**
     * @brief Checks whether a node kind provides a trait.
     *
     * @param nodeKind Node kind identifier.
     * @param trait Trait name.
     * @return True if the trait is present.
     */
    bool hasTrait(const std::string &nodeKind, const std::string &trait) const;

    /**
     * @brief Validates a node and all reachable child nodes.
     *
     * @param node Root node to validate.
     *
     * @throws std::runtime_error If validation fails.
     */
    void validate(const Node &node) const;

private:
    static bool fieldMatchesKind(const Field &field, FieldKind kind);

    void validateFieldKnown(const NodeSchema &schema, const std::string &fieldName, const Node &node) const;
    void validateRequiredFields(const NodeSchema &schema, const Node &node) const;
    void validateFieldTypes(const NodeSchema &schema, const Node &node) const;
    void validateChildConstraints(const FieldSchema &fieldSchema, const Node &child, const Node &owner) const;
    void validateChildren(const Node &node) const;

    const FieldSchema *findFieldSchema(const NodeSchema &schema, const std::string &fieldName) const;

    std::unordered_map<std::string, NodeSchema> schemas_;
    registry::DuplicatePolicy duplicatePolicy_;
};

} // namespace novac::ast