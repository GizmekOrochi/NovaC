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
    if (const auto *value{std::get_if<int>(&data_)}) {
        return *value;
    }

    throw std::runtime_error(
        "Value::asInt: value is not int");
}

double Value::asFloat() const {
    if (const auto *value{std::get_if<double>(&data_)}) {
        return *value;
    }

    if (const auto *value{std::get_if<int>(&data_)}) {
        return *value;
    }

    throw std::runtime_error(
        "Value::asFloat: value is not float");
}

bool Value::asBool() const {
    if (const auto *value{std::get_if<bool>(&data_)}) {
        return *value;
    }

    throw std::runtime_error(
        "Value::asBool: value is not bool");
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

    return "void";
}

Environment::Environment(Environment *parent)
    : parent_{parent},
      values_{} {}

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

    if (iter != values_.end()) {
        return &iter->second;
    }

    if (parent_) {
        return parent_->resolve(name);
    }

    return nullptr;
}

RuntimeContext::RuntimeContext(const RuntimeRegistry &registry)
    : registry_{registry},
      scopes_{},
      functions_{},
      hasReturn_{false},
      returnValue_{}  {
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
    if (scopes_.empty()) {
        throw std::runtime_error(
            "RuntimeContext::popScope: no scope to pop");
    }

    scopes_.pop_back();
}

Environment &RuntimeContext::env() {
    if (scopes_.empty()) {
        throw std::runtime_error(
            "RuntimeContext::env: no active scope");
    }

    return *scopes_.back();
}

const RuntimeRegistry &RuntimeContext::registry() const {
    return registry_;
}

void RuntimeContext::registerFunction(const ast::NodePtr &function) {
    functions_[function->str("name")] = function;
}

ast::NodePtr RuntimeContext::function(const std::string &name) const {
    const auto iter{functions_.find(name)};

    if (iter == functions_.end()) {
        return nullptr;
    }

    return iter->second;
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

void RuntimeRegistry::expression(std::string kind, ExprHandler handler) {
    expressions_[std::move(kind)] = std::move(handler);
}

void RuntimeRegistry::statement(std::string kind, StmtHandler handler) {
    statements_[std::move(kind)] = std::move(handler);
}

void RuntimeRegistry::declaration(std::string kind, DeclHandler handler) {
    declarations_[std::move(kind)] = std::move(handler);
}

void RuntimeRegistry::binaryOperator(std::string op, BinaryHandler handler) {
    if (!binaryDispatcherInstalled_) {
        expressions_["binary"] = [](const ast::Node &node, const RuntimeContext &context) {
            return context.registry().evalBinary(node, context);
        };

        binaryDispatcherInstalled_ = true;
    }

    binaryOperators_[std::move(op)] = std::move(handler);
}

Value RuntimeRegistry::evalBinary(const ast::Node &node, const RuntimeContext &context) const {
    const std::string op{node.str("op")};
    const auto iter{binaryOperators_.find(op)};

    if (iter == binaryOperators_.end()) {
        throw std::runtime_error(
            "RuntimeRegistry::evalBinary: no runtime binary operator handler for '" + op + "'");
    }

    return iter->second(node, context);
}

Value RuntimeRegistry::eval(const ast::Node &node, const RuntimeContext &context) const {
    const auto iter{expressions_.find(node.kind())};

    if (iter == expressions_.end()) {
        throw std::runtime_error(
            "RuntimeRegistry::eval: no runtime expression handler for '" + node.kind() + "'");
    }

    return iter->second(node, context);
}

void RuntimeRegistry::exec(const ast::Node &node, RuntimeContext &context) const {
    const auto iter{statements_.find(node.kind())};

    if (iter == statements_.end()) {
        throw std::runtime_error(
            "RuntimeRegistry::exec: no runtime statement handler for '" + node.kind() + "'");
    }

    iter->second(node, context);
}

void RuntimeRegistry::declare(const ast::NodePtr &node, RuntimeContext &context) const {
    const auto iter{declarations_.find(node->kind())};

    if (iter != declarations_.end()) {
        iter->second(node, context);
    }
}

Runtime::Runtime(const RuntimeRegistry &registry)
    : registry_{registry} {}

Value Runtime::run(const ast::Node &program) const {
    RuntimeContext context{registry_};

    for (const ast::NodePtr &declaration : program.list("declarations")) {
        registry_.declare(declaration, context);
    }

    const ast::NodePtr main{context.function("main")};

    if (!main) {
        throw std::runtime_error(
            "Runtime::run: missing main function");
    }

    const ast::NodePtr body{main->child("body")};

    context.pushScope();
    context.exec(*body);
    context.popScope();

    if (context.hasReturn()) {
        return context.takeReturn();
    }

    return Value::voidValue();
}

} // namespace novac::runtime