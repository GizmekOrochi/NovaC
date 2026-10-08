#include "novac/engine/syntax/Node.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace novac::ast {

/**
 * @brief Constructs a `Node` instance.
 *
 * @param kind Value supplied for `kind`.
 */
Node::Node(std::string kind)
    : kind_{std::move(kind)}, fields_{}, span_{}, metadata_{} {
    if (kind_.empty()) {
        throw std::runtime_error("Node::Node: kind cannot be empty");
    }
}

/**
 * @brief Constructs a `Node` instance.
 *
 * @param kind Value supplied for `kind`.
 */
Node::Node(const ids::NodeKind &kind)
    : Node{kind.value} {}

/**
 * @brief Creates a value through `make`.
 *
 * @param kind Value supplied for `kind`.
 * @return Value produced by the operation.
 */
NodePtr Node::make(std::string kind) {
    return std::make_shared<Node>(std::move(kind));
}

/**
 * @brief Creates a value through `make`.
 *
 * @param kind Value supplied for `kind`.
 * @return Value produced by the operation.
 */
NodePtr Node::make(const ids::NodeKind &kind) {
    return std::make_shared<Node>(kind);
}

/**
 * @brief Returns the value exposed by `kind`.
 *
 * @return Value produced by the operation.
 */
const std::string &Node::kind() const {
    return kind_;
}

/**
 * @brief Sets the value handled by `set`.
 *
 * @param name Value supplied for `name`.
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
Node &Node::set(std::string name, Field value) {
    if (name.empty()) {
        throw std::runtime_error("Node::set: field name cannot be empty");
    }

    fields_[std::move(name)] = std::move(value);

    return *this;
}

/**
 * @brief Sets the value handled by `set`.
 *
 * @param name Value supplied for `name`.
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
Node &Node::set(const ids::FieldName &name, Field value) {
    return set(name.value, std::move(value));
}

/**
 * @brief Checks the condition represented by `has`.
 *
 * @param name Value supplied for `name`.
 * @return Value produced by the operation.
 */
bool Node::has(const std::string &name) const {
    return fields_.find(name) != fields_.end();
}

/**
 * @brief Checks the condition represented by `has`.
 *
 * @param name Value supplied for `name`.
 * @return Value produced by the operation.
 */
bool Node::has(const ids::FieldName &name) const {
    return has(name.value);
}

/**
 * @brief Implements the `field` operation.
 *
 * @param name Value supplied for `name`.
 * @return Value produced by the operation.
 */
const Field &Node::field(const std::string &name) const {
    const auto iter{fields_.find(name)};

    if (iter == fields_.end()) {
        throw std::runtime_error("Node::field: invalid field name '" + name + "'");
    }

    return iter->second;
}

/**
 * @brief Implements the `field` operation.
 *
 * @param name Value supplied for `name`.
 * @return Value produced by the operation.
 */
const Field &Node::field(const ids::FieldName &name) const {
    return field(name.value);
}

/**
 * @brief Returns the value exposed by `fields`.
 *
 * @return Value produced by the operation.
 */
const std::unordered_map<std::string, Field> &Node::fields() const {
    return fields_;
}

/**
 * @brief Implements the `str` operation.
 *
 * @param name Value supplied for `name`.
 * @return Value produced by the operation.
 */
const std::string &Node::str(const std::string &name) const {
    const Field &value{field(name)};

    if (!std::holds_alternative<std::string>(value)) {
        throw std::runtime_error("Node::str: field '" + name + "' is not a string");
    }

    return std::get<std::string>(value);
}

/**
 * @brief Implements the `str` operation.
 *
 * @param name Value supplied for `name`.
 * @return Value produced by the operation.
 */
const std::string &Node::str(const ids::FieldName &name) const {
    return str(name.value);
}

/**
 * @brief Implements the `integer` operation.
 *
 * @param name Value supplied for `name`.
 * @return Value produced by the operation.
 */
int Node::integer(const std::string &name) const {
    const Field &value{field(name)};

    if (!std::holds_alternative<int>(value)) {
        throw std::runtime_error("Node::integer: field '" + name + "' is not an integer");
    }

    return std::get<int>(value);
}

/**
 * @brief Implements the `integer` operation.
 *
 * @param name Value supplied for `name`.
 * @return Value produced by the operation.
 */
int Node::integer(const ids::FieldName &name) const {
    return integer(name.value);
}

/**
 * @brief Implements the `child` operation.
 *
 * @param name Value supplied for `name`.
 * @return Value produced by the operation.
 */
NodePtr Node::child(const std::string &name) const {
    const Field &value{field(name)};

    if (!std::holds_alternative<NodePtr>(value)) {
        throw std::runtime_error("Node::child: field '" + name + "' is not a node");
    }

    return std::get<NodePtr>(value);
}

/**
 * @brief Implements the `child` operation.
 *
 * @param name Value supplied for `name`.
 * @return Value produced by the operation.
 */
NodePtr Node::child(const ids::FieldName &name) const {
    return child(name.value);
}

/**
 * @brief Implements the `list` operation.
 *
 * @param name Value supplied for `name`.
 * @return Value produced by the operation.
 */
const NodeList &Node::list(const std::string &name) const {
    const Field &value{field(name)};

    if (!std::holds_alternative<NodeList>(value)) {
        throw std::runtime_error("Node::list: field '" + name + "' is not a node list");
    }

    return std::get<NodeList>(value);
}

/**
 * @brief Implements the `list` operation.
 *
 * @param name Value supplied for `name`.
 * @return Value produced by the operation.
 */
const NodeList &Node::list(const ids::FieldName &name) const {
    return list(name.value);
}

/**
 * @brief Sets the value handled by `setSpan`.
 *
 * @param span Value supplied for `span`.
 * @return Value produced by the operation.
 */
Node &Node::setSpan(diagnostics::SourceSpan span) {
    span_ = std::move(span);
    return *this;
}

/**
 * @brief Returns the value exposed by `span`.
 *
 * @return Value produced by the operation.
 */
const diagnostics::SourceSpan &Node::span() const noexcept {
    return span_;
}

/**
 * @brief Returns the value exposed by `metadata`.
 *
 * @return Value produced by the operation.
 */
metadata::MetadataStore &Node::metadata() noexcept {
    return metadata_;
}

/**
 * @brief Returns the value exposed by `metadata`.
 *
 * @return Value produced by the operation.
 */
const metadata::MetadataStore &Node::metadata() const noexcept {
    return metadata_;
}

/**
 * @brief Constructs a `NodeRegistry` instance.
 *
 * @param duplicatePolicy Value supplied for `duplicatePolicy`.
 */
NodeRegistry::NodeRegistry(registry::DuplicatePolicy duplicatePolicy)
    : schemas_{}, duplicatePolicy_{duplicatePolicy} {}

/**
 * @brief Registers data through `registerNode`.
 *
 * @param schema Value supplied for `schema`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus NodeRegistry::registerNode(NodeSchema schema) {
    if (schema.kind.empty()) {
        throw std::runtime_error("NodeRegistry::registerNode: node kind cannot be empty");
    }

    const std::string kind{schema.kind};

    return registry::registerEntry(schemas_, kind, std::move(schema), duplicatePolicy_, "NodeRegistry::registerNode");
}

/**
 * @brief Finds the value requested by `find`.
 *
 * @param kind Value supplied for `kind`.
 * @return Value produced by the operation.
 */
const NodeSchema *NodeRegistry::find(const std::string &kind) const {
    const auto iter{schemas_.find(kind)};

    if (iter == schemas_.end()) {
        return nullptr;
    }

    return &iter->second;
}

/**
 * @brief Finds the value requested by `find`.
 *
 * @param kind Value supplied for `kind`.
 * @return Value produced by the operation.
 */
const NodeSchema *NodeRegistry::find(const ids::NodeKind &kind) const {
    return find(kind.value);
}

/**
 * @brief Checks the condition represented by `hasTrait`.
 *
 * @param nodeKind Value supplied for `nodeKind`.
 * @param trait Value supplied for `trait`.
 * @return Value produced by the operation.
 */
bool NodeRegistry::hasTrait(const std::string &nodeKind, const std::string &trait) const {
    const NodeSchema *schema{find(nodeKind)};

    if (!schema) {
        return false;
    }

    return std::find(schema->traits.begin(), schema->traits.end(), trait) != schema->traits.end();
}

/**
 * @brief Validates data through `validate`.
 *
 * @param node Value supplied for `node`.
 */
void NodeRegistry::validate(const Node &node) const {
    const NodeSchema *schema{find(node.kind())};

    if (!schema) {
        throw std::runtime_error("NodeRegistry::validate: unknown node kind '" + node.kind() + "'");
    }

    for (const auto &[fieldName, field] : node.fields()) {
        static_cast<void>(field);
        validateFieldKnown(*schema, fieldName, node);
    }

    validateRequiredFields(*schema, node);
    validateFieldTypes(*schema, node);
    validateChildren(node);
}

/**
 * @brief Implements the `fieldMatchesKind` operation.
 *
 * @param field Value supplied for `field`.
 * @param kind Value supplied for `kind`.
 * @return Value produced by the operation.
 */
bool NodeRegistry::fieldMatchesKind(const Field &field, FieldKind kind) {
    if (kind == FieldKind::Any) {
        return true;
    }

    if (std::holds_alternative<std::monostate>(field)) {
        return false;
    }

    if (kind == FieldKind::Int) {
        return std::holds_alternative<int>(field);
    }

    if (kind == FieldKind::Float) {
        return std::holds_alternative<double>(field);
    }

    if (kind == FieldKind::Bool) {
        return std::holds_alternative<bool>(field);
    }

    if (kind == FieldKind::String) {
        return std::holds_alternative<std::string>(field);
    }

    if (kind == FieldKind::Node) {
        return std::holds_alternative<NodePtr>(field);
    }

    if (kind == FieldKind::NodeListField) {
        return std::holds_alternative<NodeList>(field);
    }

    if (kind == FieldKind::Opaque) {
        return std::holds_alternative<std::any>(field);
    }

    return false;
}

/**
 * @brief Validates data through `validateFieldKnown`.
 *
 * @param schema Value supplied for `schema`.
 * @param fieldName Value supplied for `fieldName`.
 * @param node Value supplied for `node`.
 */
void NodeRegistry::validateFieldKnown(const NodeSchema &schema, const std::string &fieldName, const Node &node) const {
    if (!findFieldSchema(schema, fieldName)) {
        throw std::runtime_error("NodeRegistry::validateFieldKnown: node kind '" + node.kind() + "' has unknown field '" + fieldName + "'");
    }
}

/**
 * @brief Validates data through `validateRequiredFields`.
 *
 * @param schema Value supplied for `schema`.
 * @param node Value supplied for `node`.
 */
void NodeRegistry::validateRequiredFields(const NodeSchema &schema, const Node &node) const {
    for (const FieldSchema &fieldSchema : schema.fields) {
        if (!fieldSchema.required) {
            continue;
        }

        if (!node.has(fieldSchema.name)) {
            throw std::runtime_error("NodeRegistry::validateRequiredFields: node kind '" + node.kind() + "' is missing required field '" + fieldSchema.name + "'");
        }
    }
}

/**
 * @brief Validates data through `validateFieldTypes`.
 *
 * @param schema Value supplied for `schema`.
 * @param node Value supplied for `node`.
 */
void NodeRegistry::validateFieldTypes(const NodeSchema &schema, const Node &node) const {
    for (const auto &[fieldName, field] : node.fields()) {
        const FieldSchema *fieldSchema{findFieldSchema(schema, fieldName)};

        if (!fieldSchema) {
            continue;
        }

        if (std::holds_alternative<std::monostate>(field) && !fieldSchema->required) {
            continue;
        }

        if (!fieldMatchesKind(field, fieldSchema->kind)) {
            throw std::runtime_error("NodeRegistry::validateFieldTypes: node kind '" + node.kind() + "' field '" + fieldName + "' has invalid type");
        }

        if (fieldSchema->kind == FieldKind::Node) {
            const NodePtr &child{std::get<NodePtr>(field)};

            if (!child) {
                throw std::runtime_error("NodeRegistry::validateFieldTypes: node kind '" + node.kind() + "' field '" + fieldName + "' contains null child");
            }

            validateChildConstraints(*fieldSchema, *child, node);
        }

        if (fieldSchema->kind == FieldKind::NodeListField) {
            const NodeList &children{std::get<NodeList>(field)};

            for (const NodePtr &child : children) {
                if (!child) {
                    throw std::runtime_error("NodeRegistry::validateFieldTypes: node kind '" + node.kind() + "' field '" + fieldName + "' contains null child");
                }

                validateChildConstraints(*fieldSchema, *child, node);
            }
        }
    }
}

/**
 * @brief Validates data through `validateChildConstraints`.
 *
 * @param fieldSchema Value supplied for `fieldSchema`.
 * @param child Value supplied for `child`.
 * @param owner Value supplied for `owner`.
 */
void NodeRegistry::validateChildConstraints(const FieldSchema &fieldSchema, const Node &child, const Node &owner) const {
    if (!fieldSchema.allowedNodeKinds.empty()) {
        const auto iter{std::find(fieldSchema.allowedNodeKinds.begin(), fieldSchema.allowedNodeKinds.end(), child.kind())};

        if (iter == fieldSchema.allowedNodeKinds.end()) {
            throw std::runtime_error(
                "NodeRegistry::validateChildConstraints: node kind '" + owner.kind() + "' field '" + fieldSchema.name
                + "' rejects child kind '" + child.kind() + "'");
        }
    }

    if (!fieldSchema.allowedNodeTraits.empty()) {
        bool matched{};

        for (const std::string &trait : fieldSchema.allowedNodeTraits) {
            if (hasTrait(child.kind(), trait)) {
                matched = true;
                break;
            }
        }

        if (!matched) {
            throw std::runtime_error(
                "NodeRegistry::validateChildConstraints: node kind '" + owner.kind() + "' field '" + fieldSchema.name
                + "' rejects child kind '" + child.kind() + "' because it has none of the required traits");
        }
    }
}

/**
 * @brief Validates data through `validateChildren`.
 *
 * @param node Value supplied for `node`.
 */
void NodeRegistry::validateChildren(const Node &node) const {
    for (const auto &[fieldName, field] : node.fields()) {
        static_cast<void>(fieldName);

        if (const auto *child{std::get_if<NodePtr>(&field)}) {
            if (*child) {
                validate(**child);
            }
        }

        if (const auto *children{std::get_if<NodeList>(&field)}) {
            for (const NodePtr &child : *children) {
                if (child) {
                    validate(*child);
                }
            }
        }
    }
}

/**
 * @brief Finds the value requested by `findFieldSchema`.
 *
 * @param schema Value supplied for `schema`.
 * @param fieldName Value supplied for `fieldName`.
 * @return Value produced by the operation.
 */
const FieldSchema *NodeRegistry::findFieldSchema(const NodeSchema &schema, const std::string &fieldName) const {
    for (const FieldSchema &fieldSchema : schema.fields) {
        if (fieldSchema.name == fieldName) {
            return &fieldSchema;
        }
    }

    return nullptr;
}

} // namespace novac::ast
