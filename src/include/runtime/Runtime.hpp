#pragma once

#include "../ast/Node.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace novac::runtime {

class Value;

using Array = std::vector<Value>;
using Object = std::unordered_map<std::string, Value>;

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

class RuntimeRegistry;

class RuntimeContext {
public:
    explicit RuntimeContext(const RuntimeRegistry &registry);

    Value eval(const ast::Node &node) const;
    void exec(const ast::Node &node);

    void pushScope();
    void popScope();

    Environment &env();

    const RuntimeRegistry &registry() const;

    void registerFunction(const ast::NodePtr &function);

    ast::NodePtr function(const std::string &name) const;

    void returnValue(Value value);
    bool hasReturn() const;

    Value takeReturn();

private:
    const RuntimeRegistry &registry_;
    mutable std::vector<std::unique_ptr<Environment>> scopes_;
    std::unordered_map<std::string, ast::NodePtr> functions_;
    bool hasReturn_;
    Value returnValue_;
};

using ExprHandler = std::function<Value(const ast::Node &, const RuntimeContext &)>;
using BinaryHandler = std::function<Value(const ast::Node &, const RuntimeContext &)>;
using StmtHandler = std::function<void(const ast::Node &, RuntimeContext &)>;
using DeclHandler = std::function<void(const ast::NodePtr &, RuntimeContext &)>;

class RuntimeRegistry {
public:
    void expression(std::string kind, ExprHandler handler);
    void statement(std::string kind, StmtHandler handler);
    void declaration(std::string kind, DeclHandler handler);

    void binaryOperator(std::string op, BinaryHandler handler);

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
};

class Runtime {
public:
    explicit Runtime(const RuntimeRegistry &registry);

    Value run(const ast::Node &program) const;

private:
    const RuntimeRegistry &registry_;
};

} // namespace novac::runtime