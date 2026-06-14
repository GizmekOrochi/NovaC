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

/**
 * @brief Runtime value container used by the execution engine.
 *
 * Represents dynamically typed values produced and consumed during
 * evaluation, including primitive values, arrays, objects, and a
 * dedicated void state.
 */
class Value {
public:
    using Data = std::variant<std::monostate, int, double, bool, std::string, Array, Object>;

    /**
     * @brief Constructs a void value.
     */
    Value();

    /**
     * @brief Constructs a value from raw variant storage.
     *
     * @param data Stored value representation.
     */
    explicit Value(Data data);

    /**
     * @brief Creates an integer value.
     *
     * @param value Integer value.
     * @return Runtime value containing the integer.
     */
    static Value integer(int value);

    /**
     * @brief Creates a floating-point value.
     *
     * @param value Floating-point value.
     * @return Runtime value containing the value.
     */
    static Value floating(double value);

    /**
     * @brief Creates a boolean value.
     *
     * @param value Boolean value.
     * @return Runtime value containing the value.
     */
    static Value boolean(bool value);

    /**
     * @brief Creates a string value.
     *
     * @param value String contents.
     * @return Runtime value containing the string.
     */
    static Value string(std::string value);

    /**
     * @brief Creates a void value.
     *
     * @return Void runtime value.
     */
    static Value voidValue();

    /**
     * @brief Returns the stored value as an integer.
     *
     * @return Integer value.
     *
     * @throws std::runtime_error If the value is not an integer.
     */
    int asInt() const;

    /**
     * @brief Returns the stored value as a floating-point number.
     *
     * Integer values are converted automatically.
     *
     * @return Floating-point value.
     *
     * @throws std::runtime_error If the value is neither an integer nor a float.
     */
    double asFloat() const;

    /**
     * @brief Returns the stored value as a boolean.
     *
     * @return Boolean value.
     *
     * @throws std::runtime_error If the value is not a boolean.
     */
    bool asBool() const;

    /**
     * @brief Evaluates the value using runtime truthiness rules.
     *
     * @return True if the value is considered truthy.
     */
    bool truthy() const;

    /**
     * @brief Converts the value to a human-readable string.
     *
     * @return String representation of the value.
     */
    std::string toString() const;

private:
    Data data_;
};

/**
 * @brief Lexically scoped variable environment.
 *
 * Environments may form a parent-child chain used for name resolution.
 * Values are owned by the environment in which they are defined.
 */
class Environment {
public:
    /**
     * @brief Creates an environment.
     *
     * @param parent Optional parent scope used for name lookup.
     */
    explicit Environment(Environment *parent = nullptr);

    /**
     * @brief Defines a value in the current scope.
     *
     * @param name Variable name.
     * @param value Initial value.
     * @return True if the binding was inserted, false if it already exists.
     *
     * @throws std::runtime_error If the name is empty.
     */
    bool define(std::string name, Value value);

    /**
     * @brief Assigns a value to an existing binding.
     *
     * Searches the current scope and all parent scopes.
     *
     * @param name Variable name.
     * @param value New value.
     * @return True if a matching binding was found.
     */
    bool assign(const std::string &name, Value value);

    /**
     * @brief Resolves a mutable binding.
     *
     * @param name Variable name.
     * @return Pointer to the value or nullptr if not found.
     */
    Value *resolve(const std::string &name);

    /**
     * @brief Resolves a read-only binding.
     *
     * @param name Variable name.
     * @return Pointer to the value or nullptr if not found.
     */
    const Value *resolve(const std::string &name) const;

private:
    Environment *parent_;
    std::unordered_map<std::string, Value> values_;
};

using ExprHandler = std::function<Value(const ast::Node &, RuntimeContext &)>;
using BinaryHandler = std::function<Value(const ast::Node &, RuntimeContext &)>;
using StmtHandler = std::function<void(const ast::Node &, RuntimeContext &)>;
using DeclHandler = std::function<void(const ast::NodePtr &, RuntimeContext &)>;

/**
 * @brief Execution context shared by runtime handlers.
 *
 * Maintains active scopes, declaration bindings, and function return state.
 */
class RuntimeContext {
public:
    /**
     * @brief Creates a runtime context.
     *
     * @param registry Runtime registry used for dispatch.
     */
    explicit RuntimeContext(const RuntimeRegistry &registry);

    /**
     * @brief Evaluates an expression node.
     *
     * @param node Expression node.
     * @return Evaluation result.
     */
    Value eval(const ast::Node &node);

    /**
     * @brief Executes a statement node.
     *
     * @param node Statement node.
     */
    void exec(const ast::Node &node);

    /**
     * @brief Pushes a nested lexical scope.
     */
    void pushScope();

    /**
     * @brief Removes the current lexical scope.
     *
     * @throws std::runtime_error If attempting to remove the root scope.
     */
    void popScope();

    /**
     * @brief Returns the current environment.
     *
     * @return Active scope.
     *
     * @throws std::runtime_error If no scope exists.
     */
    Environment &env();

    /**
     * @brief Returns the current environment.
     *
     * @return Active scope.
     *
     * @throws std::runtime_error If no scope exists.
     */
    const Environment &env() const;

    /**
     * @brief Returns the associated runtime registry.
     *
     * @return Runtime registry.
     */
    const RuntimeRegistry &registry() const;

    /**
     * @brief Associates a name with an AST node.
     *
     * @param name Binding name.
     * @param node Bound node.
     *
     * @throws std::runtime_error If the name is empty or the node is null.
     */
    void bindNode(std::string name, ast::NodePtr node);

    /**
     * @brief Retrieves a previously bound node.
     *
     * @param name Binding name.
     * @return Bound node or nullptr if not found.
     */
    ast::NodePtr boundNode(const std::string &name) const;

    /**
     * @brief Checks whether a node binding exists.
     *
     * @param name Binding name.
     * @return True if the binding exists.
     */
    bool hasBoundNode(const std::string &name) const;

    /**
     * @brief Sets the current return value.
     *
     * @param value Return value.
     */
    void returnValue(Value value);

    /**
     * @brief Indicates whether a return value has been produced.
     *
     * @return True if a return is pending.
     */
    bool hasReturn() const;

    /**
     * @brief Retrieves and clears the pending return value.
     *
     * @return Stored return value.
     */
    Value takeReturn();

private:
    const RuntimeRegistry &registry_;
    std::vector<std::unique_ptr<Environment>> scopes_;
    BindingMap nodeBindings_;
    bool hasReturn_;
    Value returnValue_;
};

/**
 * @brief Registry of runtime handlers for AST node execution.
 *
 * Maps node kinds and operators to evaluation, execution, and declaration
 * handlers used by RuntimeContext.
 */
class RuntimeRegistry {
public:
    static constexpr const char *DefaultBinaryNodeKind{"binary"};

    /**
     * @brief Creates a runtime registry.
     *
     * @param duplicatePolicy Policy used when duplicate handlers are registered.
     */
    explicit RuntimeRegistry(
        registry::DuplicatePolicy duplicatePolicy = registry::DuplicatePolicy::Error);

    /**
     * @brief Sets the node kind used for binary operator dispatch.
     *
     * @param kind Binary expression node kind.
     *
     * @throws std::runtime_error If the kind is empty or dispatch has already been installed.
     */
    void setBinaryNodeKind(std::string kind);

    /**
     * @brief Returns the configured binary expression node kind.
     *
     * @return Node kind name.
     */
    const std::string &binaryNodeKind() const;

    /**
     * @brief Registers an expression handler.
     *
     * @param kind Node kind.
     * @param handler Evaluation handler.
     * @return Registration result.
     */
    registry::RegisterStatus expression(std::string kind, ExprHandler handler);

    registry::RegisterStatus expression(const ids::NodeKind &kind, ExprHandler handler);

    /**
     * @brief Registers a statement handler.
     *
     * @param kind Node kind.
     * @param handler Execution handler.
     * @return Registration result.
     */
    registry::RegisterStatus statement(std::string kind, StmtHandler handler);

    registry::RegisterStatus statement(const ids::NodeKind &kind, StmtHandler handler);

    /**
     * @brief Registers a declaration handler.
     *
     * @param kind Node kind.
     * @param handler Declaration handler.
     * @return Registration result.
     */
    registry::RegisterStatus declaration(std::string kind, DeclHandler handler);

    registry::RegisterStatus declaration(const ids::NodeKind &kind, DeclHandler handler);

    /**
     * @brief Registers a binary operator handler.
     *
     * @param op Operator identifier.
     * @param handler Operator implementation.
     * @return Registration result.
     */
    registry::RegisterStatus binaryOperator(std::string op, BinaryHandler handler);

    registry::RegisterStatus binaryOperator(const ids::Operation &op, BinaryHandler handler);

    /**
     * @brief Evaluates a registered binary operator node.
     *
     * @param node Binary expression node.
     * @param context Runtime context.
     * @return Evaluation result.
     */
    Value evalBinary(const ast::Node &node, RuntimeContext &context) const;

    /**
     * @brief Evaluates an expression node.
     *
     * @param node Expression node.
     * @param context Runtime context.
     * @return Evaluation result.
     */
    Value eval(const ast::Node &node, RuntimeContext &context) const;

    /**
     * @brief Executes a statement node.
     *
     * @param node Statement node.
     * @param context Runtime context.
     */
    void exec(const ast::Node &node, RuntimeContext &context) const;

    /**
     * @brief Executes a declaration node.
     *
     * @param node Declaration node.
     * @param context Runtime context.
     */
    void declare(const ast::NodePtr &node, RuntimeContext &context) const;

    /**
     * @brief Executes a declaration if a handler is available.
     *
     * @param node Declaration node.
     * @param context Runtime context.
     * @return True if a declaration handler was invoked.
     */
    bool tryDeclare(const ast::NodePtr &node, RuntimeContext &context) const;

private:
    void ensureBinaryDispatcher();

    std::unordered_map<std::string, ExprHandler> expressions_;
    std::unordered_map<std::string, StmtHandler> statements_;
    std::unordered_map<std::string, DeclHandler> declarations_;
    std::unordered_map<std::string, BinaryHandler> binaryOperators_;
    std::string binaryNodeKind_;
    bool binaryDispatcherInstalled_;
    registry::DuplicatePolicy duplicatePolicy_;
};

/**
 * @brief High-level runtime entry point.
 *
 * Creates an execution context and evaluates or executes AST roots.
 */
class Runtime {
public:
    /**
     * @brief Creates a runtime.
     *
     * @param registry Runtime handler registry.
     */
    explicit Runtime(const RuntimeRegistry &registry);

    /**
     * @brief Evaluates an AST root node.
     *
     * @param root Expression root node.
     * @return Evaluation result.
     */
    Value eval(const ast::Node &root) const;

    /**
     * @brief Executes an AST root node.
     *
     * @param root Statement root node.
     */
    void exec(const ast::Node &root) const;

private:
    const RuntimeRegistry &registry_;
};

} // namespace novac::runtime