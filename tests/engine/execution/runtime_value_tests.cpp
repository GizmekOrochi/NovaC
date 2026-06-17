#include "../../tester.hpp"
#include "novac/engine/execution/Runtime.hpp"

#include <stdexcept>
#include <string>

namespace {

using novac::runtime::Array;
using novac::runtime::Object;
using novac::runtime::Value;

template <typename Fn>
bool throwsRuntimeError(Fn &&fn) {
    try {
        fn();
    } catch (const std::runtime_error &) {
        return true;
    } catch (...) {
        return false;
    }

    return false;
}

} // namespace

TEST(Value, DefaultConstructionCreatesVoid) {
    Value value{};

    CHECK(!value.truthy());
    CHECK(value.toString() == "void");
}

TEST(Value, VoidValueCreatesVoid) {
    Value value{Value::voidValue()};

    CHECK(!value.truthy());
    CHECK(value.toString() == "void");
}

TEST(Value, CreatesIntegerValue) {
    Value value{Value::integer(42)};

    CHECK(value.asInt() == 42);
    CHECK(value.asFloat() == 42.0);
    CHECK(value.truthy());
    CHECK(value.toString() == "42");
}

TEST(Value, CreatesFloatingValue) {
    Value value{Value::floating(3.5)};

    CHECK(value.asFloat() == 3.5);
    CHECK(value.truthy());
    CHECK(value.toString().find("3.500000") != std::string::npos);
}

TEST(Value, CreatesBooleanValue) {
    Value trueValue{Value::boolean(true)};
    Value falseValue{Value::boolean(false)};

    CHECK(trueValue.asBool());
    CHECK(!falseValue.asBool());
    CHECK(trueValue.truthy());
    CHECK(!falseValue.truthy());
    CHECK(trueValue.toString() == "true");
    CHECK(falseValue.toString() == "false");
}

TEST(Value, CreatesStringValue) {
    Value value{Value::string("hello")};

    CHECK(value.truthy());
    CHECK(value.toString() == "hello");
}

TEST(Value, TruthinessRules) {
    CHECK(!Value::voidValue().truthy());

    CHECK(!Value::integer(0).truthy());
    CHECK(Value::integer(1).truthy());
    CHECK(Value::integer(-1).truthy());

    CHECK(!Value::floating(0.0).truthy());
    CHECK(Value::floating(0.5).truthy());

    CHECK(!Value::boolean(false).truthy());
    CHECK(Value::boolean(true).truthy());

    CHECK(!Value::string("").truthy());
    CHECK(Value::string("x").truthy());
}

TEST(Value, RejectsInvalidAsInt) {
    CHECK(throwsRuntimeError([]() { static_cast<void>(Value::floating(1.5).asInt()); }));
    CHECK(throwsRuntimeError([]() { static_cast<void>(Value::boolean(true).asInt()); }));
    CHECK(throwsRuntimeError([]() { static_cast<void>(Value::string("1").asInt()); }));
    CHECK(throwsRuntimeError([]() { static_cast<void>(Value::voidValue().asInt()); }));
}

TEST(Value, RejectsInvalidAsFloat) {
    CHECK(throwsRuntimeError([]() { static_cast<void>(Value::boolean(true).asFloat()); }));
    CHECK(throwsRuntimeError([]() { static_cast<void>(Value::string("1").asFloat()); }));
    CHECK(throwsRuntimeError([]() { static_cast<void>(Value::voidValue().asFloat()); }));
}

TEST(Value, RejectsInvalidAsBool) {
    CHECK(throwsRuntimeError([]() { static_cast<void>(Value::integer(1).asBool()); }));
    CHECK(throwsRuntimeError([]() { static_cast<void>(Value::string("true").asBool()); }));
    CHECK(throwsRuntimeError([]() { static_cast<void>(Value::voidValue().asBool()); }));
}

TEST(Value, ArrayToString) {
    Array array{};
    array.push_back(Value::integer(1));
    array.push_back(Value::string("two"));
    array.push_back(Value::boolean(true));

    Value value{Value::Data{array}};

    CHECK(value.truthy());
    CHECK(value.toString() == "[1, two, true]");
}

TEST(Value, EmptyArrayToString) {
    Value value{Value::Data{Array{}}};

    CHECK(value.truthy());
    CHECK(value.toString() == "[]");
}

TEST(Value, ObjectToStringContainsEntries) {
    Object object{};
    object.emplace("a", Value::integer(1));
    object.emplace("b", Value::string("two"));

    Value value{Value::Data{object}};
    const std::string text{value.toString()};

    CHECK(value.truthy());
    CHECK(text.front() == '{');
    CHECK(text.back() == '}');
    CHECK(text.find("a: 1") != std::string::npos);
    CHECK(text.find("b: two") != std::string::npos);
}
