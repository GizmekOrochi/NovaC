#include "../../include/ast/Node.hpp"

#include <stdexcept>
#include <utility>

namespace novac::ast {

Node::Node(std::string kind) : kind_{std::move(kind)}, fields_{} {}

NodePtr Node::make(std::string kind) {
    return std::make_shared<Node>(std::move(kind));
}

const std::string &Node::kind() const {
    return kind_;
}

Node &Node::set(std::string name, Field value) {
    fields_[std::move(name)] = std::move(value);

    return *this;
}

bool Node::has(const std::string &name) const {
    return fields_.find(name) != fields_.end();
}

const Field &Node::field(const std::string &name) const {
    const auto iter{fields_.find(name)};

    if (iter == fields_.end()) {
        throw std::runtime_error("Node::field: invalid field name");
    }

    return iter->second;
}

const std::unordered_map<std::string, Field> &Node::fields() const {
    return fields_;
}

std::string Node::str(const std::string &name) const {
    const Field &value{field(name)};

    if (!std::holds_alternative<std::string>(value)) {
        throw std::runtime_error("Node::str: field is not a string");
    }

    return std::get<std::string>(value);
}

int Node::integer(const std::string &name) const {
    const Field &value{field(name)};

    if (!std::holds_alternative<int>(value)) {
        throw std::runtime_error("Node::integer: field is not an integer");
    }

    return std::get<int>(value);
}

NodePtr Node::child(const std::string &name) const {
    const Field &value{field(name)};

    if (!std::holds_alternative<NodePtr>(value)) {
        throw std::runtime_error("Node::child: field is not a node");
    }

    return std::get<NodePtr>(value);
}

const NodeList &Node::list(const std::string &name) const {
    const Field &value{field(name)};

    if (!std::holds_alternative<NodeList>(value)) {
        throw std::runtime_error("Node::list: field is not a node list");
    }

    return std::get<NodeList>(value);
}

bool NodeRegistry::registerNode(NodeSchema schema) {
    const std::string kind{schema.kind};

    return schemas_.emplace(kind, std::move(schema)).second;
}

const NodeSchema *NodeRegistry::find(const std::string &kind) const {
    const auto iter{schemas_.find(kind)};

    if (iter == schemas_.end()) {
        return nullptr;
    }

    return &iter->second;
}

} // namespace novac::ast