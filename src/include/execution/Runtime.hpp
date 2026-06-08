#pragma once

#include "../syntax/Node.hpp"
#include "../foundation/Ids.hpp"
#include "../foundation/registry/Registry.hpp"
#include "../foundation/registry/RegistryHelpers.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace novac::runtime {

class Value;
class RuntimeContext;
class RuntimeRegistry;

using Array = std::vector<Value>;
using Object = std::unordered_map<std::string, Value>;
using BindingMap = std::unordered_map<std::string, ast::NodePtr>;

class Value {
public:
    using Data = std::variant<std::monostate, int, double, bool, std::string, Array, Object>;

    Value();
    explicit Value(Data data);

    static Value integer(int value);
    static Value floating(double value);
    static Value boolean(bool value);
    static Value string(std::string value);
    static Value voidValue();

    int asInt() const;
    double asFloat() const;
    bool asBool() const;
    bool truthy() const;
    std::string toString() const;

private:
    Data data_;
};

class Environment {
public:
    explicit Environment(Environment *parent = nullptr);

    bool define(std::string name, Value value);
    bool assign(const std::string &name, Value value);
    Value *resolve(const std::string &name);

private:
    Environment *parent_;
    std::unordered_map<std::string, Value> values_;
};

using ExprHandler = std::function<Value(const ast::Node &, const RuntimeContext &)>;
using BinaryHandler = std::function<Value(const ast::Node &, const RuntimeContext &)>;
using StmtHandler = std::function<void(const ast::Node &, RuntimeContext &)>;
using DeclHandler = std::function<void(const ast::NodePtr &, RuntimeContext &)>;

class RuntimeContext {
public:
    explicit RuntimeContext(const RuntimeRegistry &registry);

    Value eval(const ast::Node &node) const;
    void exec(const ast::Node &node);

    void pushScope();
    void popScope();

    Environment &env();

    const RuntimeRegistry &registry() const;

    void bindNode(std::string name, ast::NodePtr node);
    ast::NodePtr boundNode(const std::string &name) const;
    bool hasBoundNode(const std::string &name) const;

    void returnValue(Value value);
    bool hasReturn() const;
    Value takeReturn();

private:
    const RuntimeRegistry &registry_;
    mutable std::vector<std::unique_ptr<Environment>> scopes_;
    BindingMap nodeBindings_;
    bool hasReturn_;
    Value returnValue_;
};

class RuntimeRegistry {
public:
    explicit RuntimeRegistry(
        registry::DuplicatePolicy duplicatePolicy = registry::DuplicatePolicy::Error);

    registry::RegisterStatus expression(std::string kind, ExprHandler handler);
    registry::RegisterStatus expression(const ids::NodeKind &kind, ExprHandler handler);

    registry::RegisterStatus statement(std::string kind, StmtHandler handler);
    registry::RegisterStatus statement(const ids::NodeKind &kind, StmtHandler handler);

    registry::RegisterStatus declaration(std::string kind, DeclHandler handler);
    registry::RegisterStatus declaration(const ids::NodeKind &kind, DeclHandler handler);

    registry::RegisterStatus binaryOperator(std::string op, BinaryHandler handler);
    registry::RegisterStatus binaryOperator(const ids::Operation &op, BinaryHandler handler);

    Value evalBinary(const ast::Node &node, const RuntimeContext &context) const;
    Value eval(const ast::Node &node, const RuntimeContext &context) const;

    void exec(const ast::Node &node, RuntimeContext &context) const;
    void declare(const ast::NodePtr &node, RuntimeContext &context) const;

private:
    std::unordered_map<std::string, ExprHandler> expressions_;
    std::unordered_map<std::string, StmtHandler> statements_;
    std::unordered_map<std::string, DeclHandler> declarations_;
    std::unordered_map<std::string, BinaryHandler> binaryOperators_;
    bool binaryDispatcherInstalled_;
    registry::DuplicatePolicy duplicatePolicy_;
};

class Runtime {
public:
    explicit Runtime(const RuntimeRegistry &registry);

    Value eval(const ast::Node &root) const;
    void exec(const ast::Node &root) const;

private:
    const RuntimeRegistry &registry_;
};

} // namespace novac::runtime