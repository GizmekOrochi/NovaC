#include "../../include/runtime/Runtime.hpp"

#include <stdexcept>
#include <utility>

namespace novac::runtime {

Value::Value() : data_{} {}

Value::Value(Data data) : data_{std::move(data)} {}

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
    if (const auto *value{std::get_if<int>(&data_)}) return *value;
    
    throw std::runtime_error("Value::asInt: value is not int");
}

double Value::asFloat() const {
    if (const auto *value{std::get_if<double>(&data_)})return *value;
    if (const auto *value{std::get_if<int>(&data_)}) return *value;

    throw std::runtime_error("Value::asFloat: value is not float");
}

bool Value::asBool() const {
    if (const auto *value{std::get_if<bool>(&data_)})
        return *value;

    throw std::runtime_error("Value::asBool: value is not bool");
}

bool Value::truthy() const {
    if (std::holds_alternative<std::monostate>(data_)) return false;
    if (const auto *value{std::get_if<bool>(&data_)}) return *value;
    if (const auto *value{std::get_if<int>(&data_)}) return *value != 0;
    if (const auto *value{std::get_if<double>(&data_)}) return *value != 0.0;
    if (const auto *value{std::get_if<std::string>(&data_)}) return !value->empty();

    return true;
}

std::string Value::toString() const {
    if (const auto *value{std::get_if<int>(&data_)}) return std::to_string(*value);
    if (const auto *value{std::get_if<double>(&data_)}) return std::to_string(*value);
    if (const auto *value{std::get_if<bool>(&data_)}) return *value ? "true" : "false";
    if (const auto *value{std::get_if<std::string>(&data_)}) return *value;

    return "void";
}

Environment::Environment(Environment *parent)
    : parent_{parent}, values_{} {}

bool Environment::define(std::string name, Value value) {
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

    if (iter != values_.end())
        return &iter->second;

    if (parent_)
        return parent_->resolve(name);

    return nullptr;
}

RuntimeContext::RuntimeContext(const RuntimeRegistry &registry)
    : registry_{registry}, scopes_{}, nodeBindings_{}, hasReturn_{false}, returnValue_{} {
    scopes_.push_back(std::make_unique<Environment>(nullptr));
}

Value RuntimeContext::eval(const ast::Node &node) const {
    return registry_.eval(node, *this);
}

void RuntimeContext::exec(const ast::Node &node) {
    registry_.exec(node, *this);
}

void RuntimeContext::pushScope() {
    scopes_.push_back(std::make_unique<Environment>(scopes_.back().get()));
}

void RuntimeContext::popScope() {
    if (scopes_.size() <= 1)
        throw std::runtime_error("RuntimeContext::popScope: cannot pop root scope");

    scopes_.pop_back();
}

Environment &RuntimeContext::env() {
    if (scopes_.empty())
        throw std::runtime_error("RuntimeContext::env: no active scope");

    return *scopes_.back();
}

const RuntimeRegistry &RuntimeContext::registry() const {
    return registry_;
}

void RuntimeContext::bindNode(std::string name, ast::NodePtr node) {
    nodeBindings_[std::move(name)] = std::move(node);
}

ast::NodePtr RuntimeContext::boundNode(const std::string &name) const {
    const auto iter{nodeBindings_.find(name)};

    if (iter == nodeBindings_.end())
        return nullptr;

    return iter->second;
}

bool RuntimeContext::hasBoundNode(const std::string &name) const
{
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
    return returnValue_;
}

template<class Map, class Value>
registry::RegisterStatus registerEntry(Map &map, std::string key, Value value, registry::DuplicatePolicy duplicatePolicy, const std::string &owner) {
    const auto iter{map.find(key)};

    if (iter != map.end()) {
        if (duplicatePolicy == registry::DuplicatePolicy::Ignore)
            return registry::RegisterStatus::Ignored;
    
        if (duplicatePolicy == registry::DuplicatePolicy::Replace) {
            iter->second = std::move(value);

            return registry::RegisterStatus::Replaced;
        }

        throw std::runtime_error(owner + ": duplicate registration '" + key + "'");
    }

    map.emplace(std::move(key), std::move(value));

    return registry::RegisterStatus::Inserted;
}

RuntimeRegistry::RuntimeRegistry(registry::DuplicatePolicy duplicatePolicy)
    : expressions_{}, statements_{}, declarations_{}, binaryOperators_{}, binaryDispatcherInstalled_{false}, duplicatePolicy_{duplicatePolicy} {}

registry::RegisterStatus RuntimeRegistry::expression(std::string kind, ExprHandler handler) {
    return registerEntry(expressions_, std::move(kind), std::move(handler), duplicatePolicy_, "RuntimeRegistry::expression");
}

registry::RegisterStatus RuntimeRegistry::statement(std::string kind, StmtHandler handler) {
    return registerEntry(statements_, std::move(kind), std::move(handler), duplicatePolicy_, "RuntimeRegistry::statement");
}

registry::RegisterStatus RuntimeRegistry::declaration(std::string kind, DeclHandler handler) {
    return registerEntry(declarations_, std::move(kind), std::move(handler), duplicatePolicy_, "RuntimeRegistry::declaration");
}

registry::RegisterStatus RuntimeRegistry::binaryOperator(std::string op, BinaryHandler handler) {
    if (!binaryDispatcherInstalled_) {
        expressions_.emplace("binary", [](const ast::Node &node, const RuntimeContext &context) { return context.registry().evalBinary(node, context); });
        binaryDispatcherInstalled_ = true;
    }

    return registerEntry(binaryOperators_, std::move(op), std::move(handler), duplicatePolicy_, "RuntimeRegistry::binaryOperator");
}

Value RuntimeRegistry::evalBinary(const ast::Node &node, const RuntimeContext &context) const {
    const std::string op{node.str("op")};
    const auto iter{binaryOperators_.find(op)};

    if (iter == binaryOperators_.end())
        throw std::runtime_error("RuntimeRegistry::evalBinary: no runtime binary operator handler for '" + op + "'");

    return iter->second(node, context);
}

Value RuntimeRegistry::eval(const ast::Node &node, const RuntimeContext &context) const {
    const auto iter{expressions_.find(node.kind())};

    if (iter == expressions_.end())
        throw std::runtime_error("RuntimeRegistry::eval: no runtime expression handler for '" + node.kind() + "'");

    return iter->second(node, context);
}

void RuntimeRegistry::exec(const ast::Node &node, RuntimeContext &context) const {
    const auto iter{statements_.find(node.kind())};

    if (iter == statements_.end())
        throw std::runtime_error("RuntimeRegistry::exec: no runtime statement handler for '" + node.kind() + "'");

    iter->second(node, context);
}

void RuntimeRegistry::declare(const ast::NodePtr &node, RuntimeContext &context) const {
    const auto iter{declarations_.find(node->kind())};

    if (iter != declarations_.end())
        iter->second(node, context);
}

Runtime::Runtime(const RuntimeRegistry &registry) : registry_{registry} {}

Value Runtime::eval(const ast::Node &root) const {
    RuntimeContext context{registry_};

    return context.eval(root);
}

void Runtime::exec(const ast::Node &root) const {
    RuntimeContext context{registry_};

    context.exec(root);
}

} // namespace novac::runtime