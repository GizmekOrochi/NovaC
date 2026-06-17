#include "../../tester.hpp"
#include "novac/engine/execution/Runtime.hpp"
#include "novac/engine/syntax/Node.hpp"

#include <stdexcept>
#include <string>

namespace {

using novac::ast::Node;
using novac::ast::NodePtr;
using novac::runtime::RuntimeContext;
using novac::runtime::RuntimeRegistry;
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

NodePtr node(std::string kind) {
    return Node::make(std::move(kind));
}

} // namespace

TEST(RuntimeContext, StartsWithRootEnvironment) {
    RuntimeRegistry registry{};
    RuntimeContext context{registry};

    CHECK(context.env().resolve("x") == nullptr);
}

TEST(RuntimeContext, PushScopeCreatesNestedEnvironment) {
    RuntimeRegistry registry{};
    RuntimeContext context{registry};

    context.env().define("x", Value::integer(1));
    context.pushScope();

    CHECK(context.env().resolve("x") != nullptr);
    CHECK(context.env().resolve("x")->asInt() == 1);

    context.env().define("y", Value::integer(2));
    CHECK(context.env().resolve("y")->asInt() == 2);
}

TEST(RuntimeContext, PopScopeRestoresParentEnvironment) {
    RuntimeRegistry registry{};
    RuntimeContext context{registry};

    context.env().define("x", Value::integer(1));
    context.pushScope();
    context.env().define("y", Value::integer(2));

    CHECK(context.env().resolve("y") != nullptr);

    context.popScope();

    CHECK(context.env().resolve("x") != nullptr);
    CHECK(context.env().resolve("y") == nullptr);
}

TEST(RuntimeContext, RejectsPoppingRootScope) {
    RuntimeRegistry registry{};
    RuntimeContext context{registry};

    CHECK(throwsRuntimeError([&]() { context.popScope(); }));
}

TEST(RuntimeContext, ReturnsAssociatedRegistry) {
    RuntimeRegistry registry{};
    RuntimeContext context{registry};

    CHECK(&context.registry() == &registry);
}

TEST(RuntimeContext, BindsAndFindsNodes) {
    RuntimeRegistry registry{};
    RuntimeContext context{registry};

    NodePtr bound{node("Function")};

    context.bindNode("foo", bound);

    CHECK(context.hasBoundNode("foo"));
    CHECK(context.boundNode("foo") == bound);
    CHECK(context.boundNode("bar") == nullptr);
}

TEST(RuntimeContext, ReplacesExistingNodeBinding) {
    RuntimeRegistry registry{};
    RuntimeContext context{registry};

    NodePtr first{node("First")};
    NodePtr second{node("Second")};

    context.bindNode("x", first);
    context.bindNode("x", second);

    CHECK(context.boundNode("x") == second);
}

TEST(RuntimeContext, RejectsInvalidNodeBindings) {
    RuntimeRegistry registry{};
    RuntimeContext context{registry};

    CHECK(throwsRuntimeError([&]() { context.bindNode("", node("Node")); }));
    CHECK(throwsRuntimeError([&]() { context.bindNode("x", nullptr); }));
}

TEST(RuntimeContext, ReturnValueLifecycle) {
    RuntimeRegistry registry{};
    RuntimeContext context{registry};

    CHECK(!context.hasReturn());

    context.returnValue(Value::integer(42));

    CHECK(context.hasReturn());

    Value value{context.takeReturn()};

    CHECK(value.asInt() == 42);
    CHECK(!context.hasReturn());
    CHECK(context.takeReturn().toString() == "void");
}

TEST(RuntimeContext, EvalDelegatesToRegistry) {
    RuntimeRegistry registry{};
    registry.expression("Literal", [](const Node &, RuntimeContext &) { return Value::integer(7); });

    RuntimeContext context{registry};
    Node root{"Literal"};

    CHECK(context.eval(root).asInt() == 7);
}

TEST(RuntimeContext, ExecDelegatesToRegistry) {
    RuntimeRegistry registry{};
    bool called{false};

    registry.statement("Stmt", [&](const Node &, RuntimeContext &) { called = true; });

    RuntimeContext context{registry};
    Node root{"Stmt"};

    context.exec(root);

    CHECK(called);
}
