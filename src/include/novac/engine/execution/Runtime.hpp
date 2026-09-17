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

/**
 * @brief Runtime array value.
 *
 * Arrays store an ordered collection of runtime values.
 */
using Array = std::vector<Value>;

/**
 * @brief Runtime object value.
 *
 * Objects map string keys to runtime values.
 */
using Object = std::unordered_map<std::string, Value>;

/**
 * @brief Maps names to AST nodes kept by the runtime.
 *
 * This is mainly used for declarations that need to keep their original AST
 * node available during execution.
 */
using BindingMap = std::unordered_map<std::string, ast::NodePtr>;

/**
 * @brief Runtime value container used by the execution engine.
 *
 * Value represents dynamically typed data produced and consumed while
 * evaluating the AST. It can store primitive values, arrays, objects, or a
 * dedicated void state represented by std::monostate.
 */
class Value {
public:
    /**
     * @brief Variant containing the supported runtime value types.
     */
    using Data = std::variant<std::monostate, int, double, bool, std::string, Array, Object>;

    /**
     * @brief Constructs a void value.
     *
     * The default state stores std::monostate.
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
     * Integer values are accepted and converted automatically to double.
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
     * Void, false, zero numbers and empty strings are considered false.
     * Other values, including arrays and objects, are considered true.
     *
     * @return True if the value is considered truthy.
     */
    bool truthy() const;

    /**
     * @brief Converts the value to a human-readable string.
     *
     * Arrays and objects are formatted recursively using the string
     * representation of their contained values.
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
 * An environment owns variables declared in one scope and can reference a
 * parent environment. Variable lookup and assignment walk this parent chain
 * until a matching binding is found.
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
     * Only the current environment is checked for duplicates. Parent scopes do
     * not prevent a new local binding from being created.
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
     * Resolution begins in the current scope and continues through parent
     * scopes. The first matching variable is replaced.
     *
     * @param name Variable name.
     * @param value New value.
     * @return True if a matching binding was found.
     */
    bool assign(const std::string &name, Value value);

    /**
     * @brief Resolves a mutable binding.
     *
     * The current environment is searched first, followed recursively by each
     * parent environment.
     *
     * @param name Variable name.
     * @return Pointer to the value or nullptr if not found.
     */
    Value *resolve(const std::string &name);

    /**
     * @brief Resolves a read-only binding.
     *
     * The current environment is searched first, followed recursively by each
     * parent environment.
     *
     * @param name Variable name.
     * @return Pointer to the value or nullptr if not found.
     */
    const Value *resolve(const std::string &name) const;

private:
    Environment *parent_;
    std::unordered_map<std::string, Value> values_;
};

/**
 * @brief Handler used to evaluate expression nodes.
 */
using ExprHandler = std::function<Value(const ast::Node &, RuntimeContext &)>;

/**
 * @brief Handler used to evaluate binary expression nodes.
 */
using BinaryHandler = std::function<Value(const ast::Node &, RuntimeContext &)>;

/**
 * @brief Handler used to execute statement nodes.
 */
using StmtHandler = std::function<void(const ast::Node &, RuntimeContext &)>;

/**
 * @brief Handler used to process declaration nodes.
 */
using DeclHandler = std::function<void(const ast::NodePtr &, RuntimeContext &)>;

/**
 * @brief Execution context shared by runtime handlers.
 *
 * RuntimeContext keeps the state required while executing one AST: lexical
 * scopes, declaration bindings, return state and access to the RuntimeRegistry.
 *
 * Expression and statement evaluation are forwarded to the associated registry,
 * allowing language features to provide their own runtime behavior.
 */
class RuntimeContext {
public:
    /**
     * @brief Creates a runtime context.
     *
     * A root lexical environment is created automatically.
     *
     * @param registry Runtime registry used for dispatch.
     */
    explicit RuntimeContext(const RuntimeRegistry &registry);

    /**
     * @brief Evaluates an expression node.
     *
     * Evaluation is forwarded to RuntimeRegistry, which selects a handler using
     * the node kind.
     *
     * @param node Expression node.
     * @return Evaluation result.
     */
    Value eval(const ast::Node &node);

    /**
     * @brief Executes a statement node.
     *
     * Execution is forwarded to RuntimeRegistry, which selects a statement
     * handler using the node kind.
     *
     * @param node Statement node.
     */
    void exec(const ast::Node &node);

    /**
     * @brief Pushes a nested lexical scope.
     *
     * The new environment uses the previous active scope as its parent.
     */
    void pushScope();

    /**
     * @brief Removes the current lexical scope.
     *
     * The root scope is kept for the complete lifetime of the context and
     * cannot be removed.
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
     * Existing bindings using the same name are replaced.
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
     * Calling this marks a return as pending until takeReturn() consumes it.
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
     * The stored value is moved out, the return flag is reset and the internal
     * value returns to the void state.
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
 * RuntimeRegistry maps AST node kinds to expression, statement and declaration
 * handlers. Binary operators are stored separately and dispatched through a
 * configurable binary expression node kind.
 *
 * This allows language features to extend runtime behavior without modifying
 * RuntimeContext or Runtime directly.
 */
class RuntimeRegistry {
public:
    /**
     * @brief Default AST node kind used for binary expressions.
     */
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
     * The value can only be changed before the first binary operator is
     * registered, because registering an operator installs the dispatcher.
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
     * Runtime evaluation uses the AST node kind to select this handler.
     *
     * @param kind Node kind.
     * @param handler Evaluation handler.
     * @return Registration result.
     */
    registry::RegisterStatus expression(std::string kind, ExprHandler handler);

    /**
     * @brief Registers an expression handler using a typed node kind.
     */
    registry::RegisterStatus expression(const ids::NodeKind &kind, ExprHandler handler);

    /**
     * @brief Registers a statement handler.
     *
     * Runtime execution uses the AST node kind to select this handler.
     *
     * @param kind Node kind.
     * @param handler Execution handler.
     * @return Registration result.
     */
    registry::RegisterStatus statement(std::string kind, StmtHandler handler);

    /**
     * @brief Registers a statement handler using a typed node kind.
     */
    registry::RegisterStatus statement(const ids::NodeKind &kind, StmtHandler handler);

    /**
     * @brief Registers a declaration handler.
     *
     * Declaration handlers are stored separately from statements so callers
     * can explicitly dispatch declarations when needed.
     *
     * @param kind Node kind.
     * @param handler Declaration handler.
     * @return Registration result.
     */
    registry::RegisterStatus declaration(std::string kind, DeclHandler handler);

    /**
     * @brief Registers a declaration handler using a typed node kind.
     */
    registry::RegisterStatus declaration(const ids::NodeKind &kind, DeclHandler handler);

    /**
     * @brief Registers a binary operator handler.
     *
     * On the first registration, a generic expression dispatcher is installed
     * for binaryNodeKind(). That dispatcher reads the node's "op" field and
     * forwards evaluation to evalBinary().
     *
     * @param op Operator identifier.
     * @param handler Operator implementation.
     * @return Registration result.
     */
    registry::RegisterStatus binaryOperator(std::string op, BinaryHandler handler);

    /**
     * @brief Registers a binary operator using a typed operation identifier.
     */
    registry::RegisterStatus binaryOperator(const ids::Operation &op, BinaryHandler handler);

    /**
     * @brief Evaluates a registered binary operator node.
     *
     * The operator name is read from the node's "op" string field, then used to
     * find and invoke the corresponding binary handler.
     *
     * @param node Binary expression node.
     * @param context Runtime context.
     * @return Evaluation result.
     *
     * @throws std::runtime_error If no handler exists for the operator.
     */
    Value evalBinary(const ast::Node &node, RuntimeContext &context) const;

    /**
     * @brief Evaluates an expression node.
     *
     * The node kind is used as the lookup key for the registered expression
     * handler.
     *
     * @param node Expression node.
     * @param context Runtime context.
     * @return Evaluation result.
     *
     * @throws std::runtime_error If no expression handler exists for the node kind.
     */
    Value eval(const ast::Node &node, RuntimeContext &context) const;

    /**
     * @brief Executes a statement node.
     *
     * The node kind is used as the lookup key for the registered statement
     * handler.
     *
     * @param node Statement node.
     * @param context Runtime context.
     *
     * @throws std::runtime_error If no statement handler exists for the node kind.
     */
    void exec(const ast::Node &node, RuntimeContext &context) const;

    /**
     * @brief Executes a declaration node.
     *
     * The declaration node kind selects the registered declaration handler.
     *
     * @param node Declaration node.
     * @param context Runtime context.
     *
     * @throws std::runtime_error If node is null or no declaration handler exists.
     */
    void declare(const ast::NodePtr &node, RuntimeContext &context) const;

    /**
     * @brief Executes a declaration if a handler is available.
     *
     * Unlike declare(), this function does not throw when the node is null or no
     * declaration handler exists.
     *
     * @param node Declaration node.
     * @param context Runtime context.
     * @return True if a declaration handler was invoked.
     */
    bool tryDeclare(const ast::NodePtr &node, RuntimeContext &context) const;

private:
    /**
     * @brief Installs the generic binary expression dispatcher when needed.
     *
     * The dispatcher is installed only once and forwards binary AST nodes to
     * evalBinary().
     */
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
 * Runtime creates a fresh RuntimeContext for each top-level evaluation or
 * execution request and dispatches the supplied AST through the configured
 * RuntimeRegistry.
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
     * A new RuntimeContext is created for this call, then the root is evaluated
     * through the runtime registry.
     *
     * @param root Expression root node.
     * @return Evaluation result.
     */
    Value eval(const ast::Node &root) const;

    /**
     * @brief Executes an AST root node.
     *
     * A new RuntimeContext is created for this call, then the root is executed
     * through the runtime registry.
     *
     * @param root Statement root node.
     */
    void exec(const ast::Node &root) const;

private:
    const RuntimeRegistry &registry_;
};

} // namespace novac::runtime