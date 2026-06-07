#pragma once

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

enum class FieldKind {
    Any,
    Int,
    Float,
    Bool,
    String,
    Node,
    NodeList,
    Optional
};

struct FieldSchema {
    std::string name{};
    FieldKind kind{FieldKind::Any};
    bool required{true};
};

struct NodeSchema {
    std::string kind{};
    std::vector<FieldSchema> fields{};
    std::string doc{};
};

class Node {
public:
    explicit Node(std::string kind);
    static NodePtr make(std::string kind);
    const std::string &kind() const;

    Node &set(std::string name, Field value);
    bool has(const std::string &name) const;

    const Field &field(const std::string &name) const;
    const std::unordered_map<std::string, Field> &fields() const;

    std::string str(const std::string &name) const;
    int integer(const std::string &name) const;
    NodePtr child(const std::string &name) const;
    
    const NodeList &list(const std::string &name) const;

private:
    std::string kind_;
    std::unordered_map<std::string, Field> fields_;
};

class NodeRegistry {
public:
    bool registerNode(NodeSchema schema);

    const NodeSchema *find(const std::string &kind) const;

private:
    std::unordered_map<std::string, NodeSchema> schemas_;
};

} // namespace novac::ast