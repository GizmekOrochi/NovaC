#include "../../../../include/novac/engine/syntax/Node.hpp"

#include <stdexcept>
#include <utility>

namespace novac::ast {

Node::Node(std::string kind)
    : kind_{std::move(kind)}, fields_{} {}

Node::Node(const ids::NodeKind &kind)
    : Node{kind.value} {}

NodePtr Node::make(std::string kind) {
    return std::make_shared<Node>(std::move(kind));
}

NodePtr Node::make(const ids::NodeKind &kind) {
    return std::make_shared<Node>(kind);
}

const std::string &Node::kind() const {
    return kind_;
}

Node &Node::set(std::string name, Field value) {
    fields_[std::move(name)] = std::move(value);

    return *this;
}

Node &Node::set(const ids::FieldName &name, Field value) {
    return set(name.value, std::move(value));
}

bool Node::has(const std::string &name) const {
    return fields_.find(name) != fields_.end();
}

bool Node::has(const ids::FieldName &name) const {
    return has(name.value);
}

const Field &Node::field(const std::string &name) const {
    const auto iter{fields_.find(name)};

    if (iter == fields_.end())
        throw std::runtime_error("Node::field: invalid field name '" + name + "'");

    return iter->second;
}

const Field &Node::field(const ids::FieldName &name) const {
    return field(name.value);
}

const std::unordered_map<std::string, Field> &Node::fields() const {
    return fields_;
}

const std::string &Node::str(const std::string &name) const {
    const Field &value{field(name)};

    if (!std::holds_alternative<std::string>(value))
        throw std::runtime_error("Node::str: field '" + name + "' is not a string");

    return std::get<std::string>(value);
}

const std::string &Node::str(const ids::FieldName &name) const {
    return str(name.value);
}

int Node::integer(const std::string &name) const {
    const Field &value{field(name)};

    if (!std::holds_alternative<int>(value))
        throw std::runtime_error("Node::integer: field '" + name + "' is not an integer");

    return std::get<int>(value);
}

int Node::integer(const ids::FieldName &name) const {
    return integer(name.value);
}

NodePtr Node::child(const std::string &name) const {
    const Field &value{field(name)};

    if (!std::holds_alternative<NodePtr>(value))
        throw std::runtime_error("Node::child: field '" + name + "' is not a node");

    return std::get<NodePtr>(value);
}

NodePtr Node::child(const ids::FieldName &name) const {
    return child(name.value);
}

const NodeList &Node::list(const std::string &name) const {
    const Field &value{field(name)};

    if (!std::holds_alternative<NodeList>(value))
        throw std::runtime_error("Node::list: field '" + name + "' is not a node list");

    return std::get<NodeList>(value);
}

const NodeList &Node::list(const ids::FieldName &name) const {
    return list(name.value);
}

NodeRegistry::NodeRegistry(registry::DuplicatePolicy duplicatePolicy)
    : schemas_{}, duplicatePolicy_{duplicatePolicy} {}

registry::RegisterStatus NodeRegistry::registerNode(NodeSchema schema) {
    if (schema.kind.empty())
        throw std::runtime_error("NodeRegistry::registerNode: node kind cannot be empty");

    const std::string kind{schema.kind};

    return registry::registerEntry(schemas_, kind, std::move(schema), duplicatePolicy_, "NodeRegistry::registerNode");
}

const NodeSchema *NodeRegistry::find(const std::string &kind) const {
    const auto iter{schemas_.find(kind)};

    if (iter == schemas_.end())
        return nullptr;

    return &iter->second;
}

const NodeSchema *NodeRegistry::find(const ids::NodeKind &kind) const {
    return find(kind.value);
}

void NodeRegistry::validate(const Node &node) const {
    const NodeSchema *schema{find(node.kind())};

    if (!schema)
        throw std::runtime_error("NodeRegistry::validate: unknown node kind '" + node.kind() + "'");

    for (const auto &[fieldName, field] : node.fields()) {
        static_cast<void>(field);

        validateFieldKnown(*schema, fieldName, node);
    }

    validateRequiredFields(*schema, node);
    validateFieldTypes(*schema, node);
    validateChildren(node);
}

bool NodeRegistry::fieldMatchesKind(const Field &field, FieldKind kind) {
    if (kind == FieldKind::Any) return true;
    if (kind == FieldKind::Optional) return true;
    if (std::holds_alternative<std::monostate>(field)) return kind == FieldKind::Optional;
    if (kind == FieldKind::Int) return std::holds_alternative<int>(field);
    if (kind == FieldKind::Float) return std::holds_alternative<double>(field);
    if (kind == FieldKind::Bool) return std::holds_alternative<bool>(field);
    if (kind == FieldKind::String) return std::holds_alternative<std::string>(field);
    if (kind == FieldKind::Node) return std::holds_alternative<NodePtr>(field);
    if (kind == FieldKind::NodeList) return std::holds_alternative<NodeList>(field);

    return false;
}

void NodeRegistry::validateFieldKnown(const NodeSchema &schema, const std::string &fieldName, const Node &node) const {
    if (!findFieldSchema(schema, fieldName))
        throw std::runtime_error("NodeRegistry::validateFieldKnown: node kind '" + node.kind() + "' has unknown field '" + fieldName + "'");
}

void NodeRegistry::validateRequiredFields(const NodeSchema &schema, const Node &node) const {
    for (const FieldSchema &fieldSchema : schema.fields) {
        if (!fieldSchema.required)
            continue;

        if (!node.has(fieldSchema.name))
            throw std::runtime_error("NodeRegistry::validateRequiredFields: node kind '" + node.kind() + "' is missing required field '" + fieldSchema.name + "'");
    }
}

void NodeRegistry::validateFieldTypes(const NodeSchema &schema, const Node &node) const {
    for (const auto &[fieldName, field] : node.fields()) {
        const FieldSchema *fieldSchema{findFieldSchema(schema, fieldName)};

        if (!fieldSchema)
            continue;

        if (!fieldMatchesKind(field, fieldSchema->kind))
            throw std::runtime_error("NodeRegistry::validateFieldTypes: node kind '" + node.kind() + "' field '" + fieldName + "' has invalid type");

        if (fieldSchema->kind == FieldKind::Node) {
            const NodePtr &child{std::get<NodePtr>(field)};

            if (!child)
                throw std::runtime_error("NodeRegistry::validateFieldTypes: node kind '" + node.kind() + "' field '" + fieldName + "' contains null child");
        }

        if (fieldSchema->kind == FieldKind::NodeList) {
            const NodeList &children{std::get<NodeList>(field)};

            for (const NodePtr &child : children)
                if (!child)
                    throw std::runtime_error("NodeRegistry::validateFieldTypes: node kind '" + node.kind() + "' field '" + fieldName + "' contains null child");
        }
    }
}

void NodeRegistry::validateChildren(const Node &node) const {
    for (const auto &[fieldName, field] : node.fields()) {
        static_cast<void>(fieldName);

        if (const auto *child{std::get_if<NodePtr>(&field)})
            if (*child)
                validate(**child);

        if (const auto *children{std::get_if<NodeList>(&field)})
            for (const NodePtr &child : *children)
                if (child)
                    validate(*child);
    }
}

const FieldSchema *NodeRegistry::findFieldSchema(const NodeSchema &schema, const std::string &fieldName) const {
    for (const FieldSchema &fieldSchema : schema.fields)
        if (fieldSchema.name == fieldName)
            return &fieldSchema;

    return nullptr;
}

} // namespace novac::ast