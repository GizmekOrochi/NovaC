#include "novac/engine/execution/Runtime.hpp"

#include <stdexcept>
#include <utility>

namespace novac::runtime {

Value::Value()
    : data_{} {}

Value::Value(Data data)
    : data_{std::move(data)} {}

Value Value::integer(int value) {
    return Value{value};
}

Value Value::floating(double value) {
    return Value{value};
}

Value Value::boolean(bool value) {
    return Value{value};
}

Value Value::string(std::string value) {
    return Value{std::move(value)};
}

Value Value::voidValue() {
    return Value{};
}

int Value::asInt() const {
    if (const auto *value{std::get_if<int>(&data_)}) {
        return *value;
    }

    throw std::runtime_error("Value::asInt: value is not int");
}

double Value::asFloat() const {
    if (const auto *value{std::get_if<double>(&data_)}) {
        return *value;
    }

    if (const auto *value{std::get_if<int>(&data_)}) {
        return *value;
    }

    throw std::runtime_error("Value::asFloat: value is not float");
}

bool Value::asBool() const {
    if (const auto *value{std::get_if<bool>(&data_)}) {
        return *value;
    }

    throw std::runtime_error("Value::asBool: value is not bool");
}

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

Environment::Environment(Environment *parent)
    : parent_{parent}, values_{} {}

bool Environment::define(std::string name, Value value) {
    if (name.empty()) {
        throw std::runtime_error("Environment::define: name cannot be empty");
    }

    return values_.emplace(std::move(name), std::move(value)).second;
}

bool Environment::assign(const std::string &name, Value value) {
    if (Value *slot{resolve(name)}) {
        *slot = std::move(value);

        return true;
    }

    return false;
}

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

RuntimeContext::RuntimeContext(const RuntimeRegistry &registry)
    : registry_{registry}, scopes_{}, nodeBindings_{}, hasReturn_{false}, returnValue_{} {
    scopes_.push_back(std::make_unique<Environment>(nullptr));
}

Value RuntimeContext::eval(const ast::Node &node) {
    return registry_.eval(node, *this);
}

void RuntimeContext::exec(const ast::Node &node) {
    registry_.exec(node, *this);
}

void RuntimeContext::pushScope() {
    scopes_.push_back(std::make_unique<Environment>(scopes_.back().get()));
}

void RuntimeContext::popScope() {
    if (scopes_.size() <= 1) {
        throw std::runtime_error("RuntimeContext::popScope: cannot pop root scope");
    }

    scopes_.pop_back();
}

Environment &RuntimeContext::env() {
    if (scopes_.empty()) {
        throw std::runtime_error("RuntimeContext::env: no active scope");
    }

    return *scopes_.back();
}

const Environment &RuntimeContext::env() const {
    if (scopes_.empty()) {
        throw std::runtime_error("RuntimeContext::env: no active scope");
    }

    return *scopes_.back();
}

const RuntimeRegistry &RuntimeContext::registry() const {
    return registry_;
}

void RuntimeContext::bindNode(std::string name, ast::NodePtr node) {
    if (name.empty()) {
        throw std::runtime_error("RuntimeContext::bindNode: name cannot be empty");
    }

    if (!node) {
        throw std::runtime_error("RuntimeContext::bindNode: node cannot be null");
    }

    nodeBindings_[std::move(name)] = std::move(node);
}

ast::NodePtr RuntimeContext::boundNode(const std::string &name) const {
    const auto iter{nodeBindings_.find(name)};

    if (iter == nodeBindings_.end()) {
        return nullptr;
    }

    return iter->second;
}

bool RuntimeContext::hasBoundNode(const std::string &name) const {
    return nodeBindings_.find(name) != nodeBindings_.end();
}

void RuntimeContext::returnValue(Value value) {
    hasReturn_ = true;
    returnValue_ = std::move(value);
}

bool RuntimeContext::hasReturn() const {
    return hasReturn_;
}

Value RuntimeContext::takeReturn() {
    hasReturn_ = false;
    Value value{std::move(returnValue_)};
    returnValue_ = Value::voidValue();

    return value;
}

RuntimeRegistry::RuntimeRegistry(registry::DuplicatePolicy duplicatePolicy)
    : expressions_{}, statements_{}, declarations_{}, binaryOperators_{}, binaryNodeKind_{DefaultBinaryNodeKind}, binaryDispatcherInstalled_{false}, duplicatePolicy_{duplicatePolicy} {}

void RuntimeRegistry::setBinaryNodeKind(std::string kind) {
    if (kind.empty()) {
        throw std::runtime_error("RuntimeRegistry::setBinaryNodeKind: kind cannot be empty");
    }

    if (binaryDispatcherInstalled_) {
        throw std::runtime_error("RuntimeRegistry::setBinaryNodeKind: cannot change binary node kind after binary dispatcher installation");
    }

    binaryNodeKind_ = std::move(kind);
}

const std::string &RuntimeRegistry::binaryNodeKind() const {
    return binaryNodeKind_;
}

registry::RegisterStatus RuntimeRegistry::expression(std::string kind, ExprHandler handler) {
    if (kind.empty()) {
        throw std::runtime_error("RuntimeRegistry::expression: kind cannot be empty");
    }

    if (!handler) {
        throw std::runtime_error("RuntimeRegistry::expression: handler cannot be empty");
    }

    return registry::registerEntry(expressions_, std::move(kind), std::move(handler), duplicatePolicy_, "RuntimeRegistry::expression");
}

registry::RegisterStatus RuntimeRegistry::expression(const ids::NodeKind &kind, ExprHandler handler) {
    return expression(kind.value, std::move(handler));
}

registry::RegisterStatus RuntimeRegistry::statement(std::string kind, StmtHandler handler) {
    if (kind.empty()) {
        throw std::runtime_error("RuntimeRegistry::statement: kind cannot be empty");
    }

    if (!handler) {
        throw std::runtime_error("RuntimeRegistry::statement: handler cannot be empty");
    }

    return registry::registerEntry(statements_, std::move(kind), std::move(handler), duplicatePolicy_, "RuntimeRegistry::statement");
}

registry::RegisterStatus RuntimeRegistry::statement(const ids::NodeKind &kind, StmtHandler handler) {
    return statement(kind.value, std::move(handler));
}

registry::RegisterStatus RuntimeRegistry::declaration(std::string kind, DeclHandler handler) {
    if (kind.empty()) {
        throw std::runtime_error("RuntimeRegistry::declaration: kind cannot be empty");
    }

    if (!handler) {
        throw std::runtime_error("RuntimeRegistry::declaration: handler cannot be empty");
    }

    return registry::registerEntry(declarations_, std::move(kind), std::move(handler), duplicatePolicy_, "RuntimeRegistry::declaration");
}

registry::RegisterStatus RuntimeRegistry::declaration(const ids::NodeKind &kind, DeclHandler handler) {
    return declaration(kind.value, std::move(handler));
}

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

registry::RegisterStatus RuntimeRegistry::binaryOperator(const ids::Operation &op, BinaryHandler handler) {
    return binaryOperator(op.value, std::move(handler));
}

Value RuntimeRegistry::evalBinary(const ast::Node &node, RuntimeContext &context) const {
    const std::string op{node.str("op")};
    const auto iter{binaryOperators_.find(op)};

    if (iter == binaryOperators_.end()) {
        throw std::runtime_error("RuntimeRegistry::evalBinary: no runtime binary operator handler for '" + op + "'");
    }

    return iter->second(node, context);
}

Value RuntimeRegistry::eval(const ast::Node &node, RuntimeContext &context) const {
    const auto iter{expressions_.find(node.kind())};

    if (iter == expressions_.end()) {
        throw std::runtime_error("RuntimeRegistry::eval: no runtime expression handler for '" + node.kind() + "'");
    }

    return iter->second(node, context);
}

void RuntimeRegistry::exec(const ast::Node &node, RuntimeContext &context) const {
    const auto iter{statements_.find(node.kind())};

    if (iter == statements_.end()) {
        throw std::runtime_error("RuntimeRegistry::exec: no runtime statement handler for '" + node.kind() + "'");
    }

    iter->second(node, context);
}

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

Runtime::Runtime(const RuntimeRegistry &registry)
    : registry_{registry} {}

Value Runtime::eval(const ast::Node &root) const {
    RuntimeContext context{registry_};

    return context.eval(root);
}

void Runtime::exec(const ast::Node &root) const {
    RuntimeContext context{registry_};

    context.exec(root);
}

} // namespace novac::runtime