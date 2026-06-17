#include "../../tester.hpp"
#include "novac/engine/execution/Runtime.hpp"

#include <stdexcept>

namespace {

using novac::runtime::Environment;
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

TEST(Environment, DefinesAndResolvesValue) {
    Environment env{};

    CHECK(env.define("x", Value::integer(42)));

    Value *value{env.resolve("x")};

    CHECK(value != nullptr);
    CHECK(value->asInt() == 42);
}

TEST(Environment, RejectsEmptyDefinitionName) {
    Environment env{};

    CHECK(throwsRuntimeError([&]() { env.define("", Value::integer(1)); }));
}

TEST(Environment, DoesNotRedefineExistingLocalBinding) {
    Environment env{};

    CHECK(env.define("x", Value::integer(1)));
    CHECK(!env.define("x", Value::integer(2)));

    Value *value{env.resolve("x")};
    CHECK(value != nullptr);
    CHECK(value->asInt() == 1);
}

TEST(Environment, ReturnsNullForMissingBinding) {
    Environment env{};

    CHECK(env.resolve("missing") == nullptr);
}

TEST(Environment, AssignsExistingLocalBinding) {
    Environment env{};
    env.define("x", Value::integer(1));

    CHECK(env.assign("x", Value::integer(99)));

    Value *value{env.resolve("x")};
    CHECK(value != nullptr);
    CHECK(value->asInt() == 99);
}

TEST(Environment, DoesNotAssignMissingBinding) {
    Environment env{};

    CHECK(!env.assign("missing", Value::integer(99)));
    CHECK(env.resolve("missing") == nullptr);
}

TEST(Environment, ResolvesParentBinding) {
    Environment parent{};
    parent.define("x", Value::integer(10));

    Environment child{&parent};

    Value *value{child.resolve("x")};

    CHECK(value != nullptr);
    CHECK(value->asInt() == 10);
}

TEST(Environment, LocalBindingShadowsParent) {
    Environment parent{};
    parent.define("x", Value::integer(10));

    Environment child{&parent};
    child.define("x", Value::integer(20));

    CHECK(child.resolve("x")->asInt() == 20);
    CHECK(parent.resolve("x")->asInt() == 10);
}

TEST(Environment, AssignUpdatesParentBindingWhenNoLocalBindingExists) {
    Environment parent{};
    parent.define("x", Value::integer(10));

    Environment child{&parent};

    CHECK(child.assign("x", Value::integer(30)));
    CHECK(parent.resolve("x")->asInt() == 30);
    CHECK(child.resolve("x")->asInt() == 30);
}

TEST(Environment, ConstResolveFindsValues) {
    Environment env{};
    env.define("x", Value::integer(5));

    const Environment &constEnv{env};
    const Value *value{constEnv.resolve("x")};

    CHECK(value != nullptr);
    CHECK(value->asInt() == 5);
}
