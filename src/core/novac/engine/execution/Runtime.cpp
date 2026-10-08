#include "novac/engine/execution/Runtime.hpp"

#include <stdexcept>
#include <utility>

namespace novac::runtime {

/**
 * @brief Constructs a `Value` instance.
 */
Value::Value()
    : data_{} {}

/**
 * @brief Constructs a `Value` instance.
 *
 * @param data Value supplied for `data`.
 */
Value::Value(Data data)
    : data_{std::move(data)} {}

/**
 * @brief Implements the `integer` operation.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
Value Value::integer(int value) {
    return Value{value};
}

/**
 * @brief Implements the `floating` operation.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
Value Value::floating(double value) {
    return Value{value};
}

/**
 * @brief Implements the `boolean` operation.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
Value Value::boolean(bool value) {
    return Value{value};
}

/**
 * @brief Implements the `string` operation.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
Value Value::string(std::string value) {
    return Value{std::move(value)};
}

/**
 * @brief Implements the `voidValue` operation.
 *
 * @return Value produced by the operation.
 */
Value Value::voidValue() {
    return Value{};
}

/**
 * @brief Implements the `asInt` operation.
 *
 * @return Value produced by the operation.
 */
int Value::asInt() const {
    if (const auto *value{std::get_if<int>(&data_)}) {
        return *value;
    }

    throw std::runtime_error("Value::asInt: value is not int");
}

/**
 * @brief Implements the `asFloat` operation.
 *
 * @return Value produced by the operation.
 */
double Value::asFloat() const {
    if (const auto *value{std::get_if<double>(&data_)}) {
        return *value;
    }

    if (const auto *value{std::get_if<int>(&data_)}) {
        return *value;
    }

    throw std::runtime_error("Value::asFloat: value is not float");
}

/**
 * @brief Implements the `asBool` operation.
 *
 * @return Value produced by the operation.
 */
bool Value::asBool() const {
    if (const auto *value{std::get_if<bool>(&data_)}) {
        return *value;
    }

    throw std::runtime_error("Value::asBool: value is not bool");
}

/**
 * @brief Implements the `truthy` operation.
 *
 * @return Value produced by the operation.
 */
bool Value::truthy() const {
    if (std::holds_alternative<std::monostate>(data_)) {
        return false;
    }

    if (const auto *value{std::get_if<bool>(&data_)}) {
        return *value;
    }

    if (const auto *value{std::get_if<int>(&data_)}) {
        return *value != 0;
    }

    if (const auto *value{std::get_if<double>(&data_)}) {
        return *value != 0.0;
    }

    if (const auto *value{std::get_if<std::string>(&data_)}) {
        return !value->empty();
    }

    return true;
}

/**
 * @brief Implements the `toString` operation.
 *
 * @return Value produced by the operation.
 */
std::string Value::toString() const {
    if (const auto *value{std::get_if<int>(&data_)}) {
        return std::to_string(*value);
    }

    if (const auto *value{std::get_if<double>(&data_)}) {
        return std::to_string(*value);
    }

    if (const auto *value{std::get_if<bool>(&data_)}) {
        return *value ? "true" : "false";
    }

    if (const auto *value{std::get_if<std::string>(&data_)}) {
        return *value;
    }

    if (const auto *array{std::get_if<Array>(&data_)}) {
        std::string result{"["};

        for (std::size_t index{}; index < array->size(); ++index) {
            if (index != 0) {
                result += ", ";
            }

            result += (*array)[index].toString();
        }

        result += "]";

        return result;
    }

    if (const auto *object{std::get_if<Object>(&data_)}) {
        std::string result{"{"};
        bool first{true};

        for (const auto &[key, value] : *object) {
            if (!first) {
                result += ", ";
            }

            first = false;
            result += key + ": " + value.toString();
        }

        result += "}";

        return result;
    }

    return "void";
}

/**
 * @brief Constructs a `Environment` instance.
 *
 * @param parent Value supplied for `parent`.
 */
Environment::Environment(Environment *parent)
    : parent_{parent}, values_{} {}

/**
 * @brief Creates a value through `define`.
 *
 * @param name Value supplied for `name`.
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
bool Environment::define(std::string name, Value value) {
    if (name.empty()) {
        throw std::runtime_error("Environment::define: name cannot be empty");
    }

    return values_.emplace(std::move(name), std::move(value)).second;
}

/**
 * @brief Implements the `assign` operation.
 *
 * @param name Value supplied for `name`.
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
bool Environment::assign(const std::string &name, Value value) {
    if (Value *slot{resolve(name)}) {
        *slot = std::move(value);

        return true;
    }

    return false;
}

/**
 * @brief Resolves data through `resolve`.
 *
 * @param name Value supplied for `name`.
 * @return Value produced by the operation.
 */
Value *Environment::resolve(const std::string &name) {
    const auto iter{values_.find(name)};

    if (iter != values_.end()) {
        return &iter->second;
    }

    if (parent_) {
        return parent_->resolve(name);
    }

    return nullptr;
}

/**
 * @brief Resolves data through `resolve`.
 *
 * @param name Value supplied for `name`.
 * @return Value produced by the operation.
 */
const Value *Environment::resolve(const std::string &name) const {
    const auto iter{values_.find(name)};

    if (iter != values_.end()) {
        return &iter->second;
    }

    if (parent_) {
        return parent_->resolve(name);
    }

    return nullptr;
}

/**
 * @brief Constructs a `RuntimeContext` instance.
 *
 * @param registry Value supplied for `registry`.
 */
RuntimeContext::RuntimeContext(const RuntimeRegistry &registry)
    : registry_{registry}, scopes_{}, nodeBindings_{}, signal_{} {
    scopes_.push_back(std::make_unique<Environment>(nullptr));
}

/**
 * @brief Implements the `eval` operation.
 *
 * @param node Value supplied for `node`.
 * @return Value produced by the operation.
 */
Value RuntimeContext::eval(const ast::Node &node) {
    return registry_.eval(node, *this);
}

/**
 * @brief Implements the `exec` operation.
 *
 * @param node Value supplied for `node`.
 */
void RuntimeContext::exec(const ast::Node &node) {
    registry_.exec(node, *this);
}

/**
 * @brief Implements the `pushScope` operation.
 */
void RuntimeContext::pushScope() {
    scopes_.push_back(std::make_unique<Environment>(scopes_.back().get()));
}

/**
 * @brief Implements the `popScope` operation.
 */
void RuntimeContext::popScope() {
    if (scopes_.size() <= 1) {
        throw std::runtime_error("RuntimeContext::popScope: cannot pop root scope");
    }

    scopes_.pop_back();
}

/**
 * @brief Implements the `env` operation.
 *
 * @return Value produced by the operation.
 */
Environment &RuntimeContext::env() {
    if (scopes_.empty()) {
        throw std::runtime_error("RuntimeContext::env: no active scope");
    }

    return *scopes_.back();
}

/**
 * @brief Implements the `env` operation.
 *
 * @return Value produced by the operation.
 */
const Environment &RuntimeContext::env() const {
    if (scopes_.empty()) {
        throw std::runtime_error("RuntimeContext::env: no active scope");
    }

    return *scopes_.back();
}

/**
 * @brief Implements the `registry` operation.
 *
 * @return Value produced by the operation.
 */
const RuntimeRegistry &RuntimeContext::registry() const {
    return registry_;
}

/**
 * @brief Implements the `bindNode` operation.
 *
 * @param name Value supplied for `name`.
 * @param node Value supplied for `node`.
 */
void RuntimeContext::bindNode(std::string name, ast::NodePtr node) {
    if (name.empty()) {
        throw std::runtime_error("RuntimeContext::bindNode: name cannot be empty");
    }

    if (!node) {
        throw std::runtime_error("RuntimeContext::bindNode: node cannot be null");
    }

    nodeBindings_[std::move(name)] = std::move(node);
}

/**
 * @brief Implements the `boundNode` operation.
 *
 * @param name Value supplied for `name`.
 * @return Value produced by the operation.
 */
ast::NodePtr RuntimeContext::boundNode(const std::string &name) const {
    const auto iter{nodeBindings_.find(name)};

    if (iter == nodeBindings_.end()) {
        return nullptr;
    }

    return iter->second;
}

/**
 * @brief Checks the condition represented by `hasBoundNode`.
 *
 * @param name Value supplied for `name`.
 * @return Value produced by the operation.
 */
bool RuntimeContext::hasBoundNode(const std::string &name) const {
    return nodeBindings_.find(name) != nodeBindings_.end();
}

/**
 * @brief Implements the `signal` operation.
 *
 * @param value Value supplied for `value`.
 */
void RuntimeContext::signal(ControlSignal value) {
    if (value.kind.value.empty()) {
        throw std::runtime_error("RuntimeContext::signal: signal kind cannot be empty");
    }
    if (signal_) {
        throw std::runtime_error(
            "RuntimeContext::signal: cannot raise '" + value.kind.value
            + "' while control signal '" + signal_->kind.value + "' is pending");
    }

    signal_ = std::move(value);
}

/**
 * @brief Checks the condition represented by `hasSignal`.
 *
 * @return Value produced by the operation.
 */
bool RuntimeContext::hasSignal() const {
    return signal_.has_value();
}

/**
 * @brief Checks the condition represented by `hasSignal`.
 *
 * @param kind Value supplied for `kind`.
 * @return Value produced by the operation.
 */
bool RuntimeContext::hasSignal(const ids::ControlSignalKind &kind) const {
    return signal_.has_value() && signal_->kind.value == kind.value;
}

/**
 * @brief Implements the `controlSignal` operation.
 *
 * @return Value produced by the operation.
 */
const ControlSignal &RuntimeContext::controlSignal() const {
    if (!signal_) {
        throw std::runtime_error("RuntimeContext::controlSignal: no control-flow signal is pending");
    }

    return *signal_;
}

/**
 * @brief Implements the `takeSignal` operation.
 *
 * @return Value produced by the operation.
 */
ControlSignal RuntimeContext::takeSignal() {
    if (!signal_) {
        throw std::runtime_error("RuntimeContext::takeSignal: no control-flow signal is pending");
    }

    ControlSignal value{std::move(*signal_)};
    signal_.reset();
    return value;
}

/**
 * @brief Implements the `returnValue` operation.
 *
 * @param value Value supplied for `value`.
 */
void RuntimeContext::returnValue(Value value) {
    signal(ControlSignal{ReturnSignalKind, std::move(value)});
}

/**
 * @brief Checks the condition represented by `hasReturn`.
 *
 * @return Value produced by the operation.
 */
bool RuntimeContext::hasReturn() const {
    return hasSignal(ReturnSignalKind);
}

/**
 * @brief Implements the `takeReturn` operation.
 *
 * @return Value produced by the operation.
 */
Value RuntimeContext::takeReturn() {
    if (!hasReturn()) {
        return Value::voidValue();
    }

    return std::move(takeSignal().payload);
}

/**
 * @brief Constructs a `RuntimeRegistry` instance.
 *
 * @param duplicatePolicy Value supplied for `duplicatePolicy`.
 */
RuntimeRegistry::RuntimeRegistry(registry::DuplicatePolicy duplicatePolicy)
    : expressions_{}, statements_{}, declarations_{}, binaryOperators_{}, binaryNodeKind_{DefaultBinaryNodeKind}, binaryDispatcherInstalled_{false}, duplicatePolicy_{duplicatePolicy} {}

/**
 * @brief Sets the value handled by `setBinaryNodeKind`.
 *
 * @param kind Value supplied for `kind`.
 */
void RuntimeRegistry::setBinaryNodeKind(std::string kind) {
    if (kind.empty()) {
        throw std::runtime_error("RuntimeRegistry::setBinaryNodeKind: kind cannot be empty");
    }

    if (binaryDispatcherInstalled_) {
        throw std::runtime_error("RuntimeRegistry::setBinaryNodeKind: cannot change binary node kind after binary dispatcher installation");
    }

    binaryNodeKind_ = std::move(kind);
}

/**
 * @brief Implements the `binaryNodeKind` operation.
 *
 * @return Value produced by the operation.
 */
const std::string &RuntimeRegistry::binaryNodeKind() const {
    return binaryNodeKind_;
}

/**
 * @brief Implements the `expression` operation.
 *
 * @param kind Value supplied for `kind`.
 * @param handler Value supplied for `handler`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus RuntimeRegistry::expression(std::string kind, ExprHandler handler) {
    if (kind.empty()) {
        throw std::runtime_error("RuntimeRegistry::expression: kind cannot be empty");
    }

    if (!handler) {
        throw std::runtime_error("RuntimeRegistry::expression: handler cannot be empty");
    }

    return registry::registerEntry(expressions_, std::move(kind), std::move(handler), duplicatePolicy_, "RuntimeRegistry::expression");
}

/**
 * @brief Implements the `expression` operation.
 *
 * @param kind Value supplied for `kind`.
 * @param handler Value supplied for `handler`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus RuntimeRegistry::expression(const ids::NodeKind &kind, ExprHandler handler) {
    return expression(kind.value, std::move(handler));
}

/**
 * @brief Implements the `statement` operation.
 *
 * @param kind Value supplied for `kind`.
 * @param handler Value supplied for `handler`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus RuntimeRegistry::statement(std::string kind, StmtHandler handler) {
    if (kind.empty()) {
        throw std::runtime_error("RuntimeRegistry::statement: kind cannot be empty");
    }

    if (!handler) {
        throw std::runtime_error("RuntimeRegistry::statement: handler cannot be empty");
    }

    return registry::registerEntry(statements_, std::move(kind), std::move(handler), duplicatePolicy_, "RuntimeRegistry::statement");
}

/**
 * @brief Implements the `statement` operation.
 *
 * @param kind Value supplied for `kind`.
 * @param handler Value supplied for `handler`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus RuntimeRegistry::statement(const ids::NodeKind &kind, StmtHandler handler) {
    return statement(kind.value, std::move(handler));
}

/**
 * @brief Implements the `declaration` operation.
 *
 * @param kind Value supplied for `kind`.
 * @param handler Value supplied for `handler`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus RuntimeRegistry::declaration(std::string kind, DeclHandler handler) {
    if (kind.empty()) {
        throw std::runtime_error("RuntimeRegistry::declaration: kind cannot be empty");
    }

    if (!handler) {
        throw std::runtime_error("RuntimeRegistry::declaration: handler cannot be empty");
    }

    return registry::registerEntry(declarations_, std::move(kind), std::move(handler), duplicatePolicy_, "RuntimeRegistry::declaration");
}

/**
 * @brief Implements the `declaration` operation.
 *
 * @param kind Value supplied for `kind`.
 * @param handler Value supplied for `handler`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus RuntimeRegistry::declaration(const ids::NodeKind &kind, DeclHandler handler) {
    return declaration(kind.value, std::move(handler));
}

/**
 * @brief Ensures the invariant required by `ensureBinaryDispatcher`.
 */
void RuntimeRegistry::ensureBinaryDispatcher() {
    if (binaryDispatcherInstalled_) {
        return;
    }

    registry::registerEntry(
        expressions_,
        binaryNodeKind_,
        ExprHandler{[](const ast::Node &node, RuntimeContext &context) {
            return context.registry().evalBinary(node, context);
        }},
        duplicatePolicy_,
        "RuntimeRegistry::binaryOperator.dispatcher");

    binaryDispatcherInstalled_ = true;
}

/**
 * @brief Implements the `binaryOperator` operation.
 *
 * @param op Value supplied for `op`.
 * @param handler Value supplied for `handler`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus RuntimeRegistry::binaryOperator(std::string op, BinaryHandler handler) {
    if (op.empty()) {
        throw std::runtime_error("RuntimeRegistry::binaryOperator: operator cannot be empty");
    }

    if (!handler) {
        throw std::runtime_error("RuntimeRegistry::binaryOperator: handler cannot be empty");
    }

    ensureBinaryDispatcher();

    return registry::registerEntry(binaryOperators_, std::move(op), std::move(handler), duplicatePolicy_, "RuntimeRegistry::binaryOperator");
}

/**
 * @brief Implements the `binaryOperator` operation.
 *
 * @param op Value supplied for `op`.
 * @param handler Value supplied for `handler`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus RuntimeRegistry::binaryOperator(const ids::Operation &op, BinaryHandler handler) {
    return binaryOperator(op.value, std::move(handler));
}

/**
 * @brief Implements the `evalBinary` operation.
 *
 * @param node Value supplied for `node`.
 * @param context Value supplied for `context`.
 * @return Value produced by the operation.
 */
Value RuntimeRegistry::evalBinary(const ast::Node &node, RuntimeContext &context) const {
    const std::string op{node.str("op")};
    const auto iter{binaryOperators_.find(op)};

    if (iter == binaryOperators_.end()) {
        throw std::runtime_error("RuntimeRegistry::evalBinary: no runtime binary operator handler for '" + op + "'");
    }

    return iter->second(node, context);
}

/**
 * @brief Implements the `eval` operation.
 *
 * @param node Value supplied for `node`.
 * @param context Value supplied for `context`.
 * @return Value produced by the operation.
 */
Value RuntimeRegistry::eval(const ast::Node &node, RuntimeContext &context) const {
    const auto iter{expressions_.find(node.kind())};

    if (iter == expressions_.end()) {
        throw std::runtime_error("RuntimeRegistry::eval: no runtime expression handler for '" + node.kind() + "'");
    }

    return iter->second(node, context);
}

/**
 * @brief Implements the `exec` operation.
 *
 * @param node Value supplied for `node`.
 * @param context Value supplied for `context`.
 */
void RuntimeRegistry::exec(const ast::Node &node, RuntimeContext &context) const {
    const auto iter{statements_.find(node.kind())};

    if (iter == statements_.end()) {
        throw std::runtime_error("RuntimeRegistry::exec: no runtime statement handler for '" + node.kind() + "'");
    }

    iter->second(node, context);
}

/**
 * @brief Implements the `declare` operation.
 *
 * @param node Value supplied for `node`.
 * @param context Value supplied for `context`.
 */
void RuntimeRegistry::declare(const ast::NodePtr &node, RuntimeContext &context) const {
    if (!node) {
        throw std::runtime_error("RuntimeRegistry::declare: node cannot be null");
    }

    const auto iter{declarations_.find(node->kind())};

    if (iter == declarations_.end()) {
        throw std::runtime_error("RuntimeRegistry::declare: no runtime declaration handler for '" + node->kind() + "'");
    }

    iter->second(node, context);
}

/**
 * @brief Implements the `tryDeclare` operation.
 *
 * @param node Value supplied for `node`.
 * @param context Value supplied for `context`.
 * @return Value produced by the operation.
 */
bool RuntimeRegistry::tryDeclare(const ast::NodePtr &node, RuntimeContext &context) const {
    if (!node) {
        return false;
    }

    const auto iter{declarations_.find(node->kind())};

    if (iter == declarations_.end()) {
        return false;
    }

    iter->second(node, context);

    return true;
}

/**
 * @brief Constructs a `Runtime` instance.
 *
 * @param registry Value supplied for `registry`.
 */
Runtime::Runtime(const RuntimeRegistry &registry)
    : registry_{registry} {}

/**
 * @brief Implements the `eval` operation.
 *
 * @param root Value supplied for `root`.
 * @return Value produced by the operation.
 */
Value Runtime::eval(const ast::Node &root) const {
    RuntimeContext context{registry_};
    Value result{context.eval(root)};

    if (context.hasSignal()) {
        throw std::runtime_error(
            "Runtime::eval: unhandled control signal '"
            + context.controlSignal().kind.value + "'");
    }

    return result;
}

/**
 * @brief Implements the `exec` operation.
 *
 * @param root Value supplied for `root`.
 */
void Runtime::exec(const ast::Node &root) const {
    RuntimeContext context{registry_};
    context.exec(root);

    if (context.hasSignal()) {
        throw std::runtime_error(
            "Runtime::exec: unhandled control signal '"
            + context.controlSignal().kind.value + "'");
    }
}

} // namespace novac::runtime
