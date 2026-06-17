#include "../../tester.hpp"
#include "novac/engine/execution/Runtime.hpp"
#include "novac/engine/syntax/Node.hpp"

namespace {

using novac::ast::Node;
using novac::runtime::Runtime;
using novac::runtime::RuntimeContext;
using novac::runtime::RuntimeRegistry;
using novac::runtime::Value;

} // namespace

TEST(Runtime, EvalCreatesContextAndEvaluatesRoot) {
    RuntimeRegistry registry{};

    registry.expression("Literal", [](const Node &, RuntimeContext &context) {
        CHECK(context.env().resolve("temporary") == nullptr);
        context.env().define("temporary", Value::integer(1));
        return Value::integer(42);
    });

    Runtime runtime{registry};
    Node root{"Literal"};

    CHECK(runtime.eval(root).asInt() == 42);
}

TEST(Runtime, ExecCreatesContextAndExecutesRoot) {
    RuntimeRegistry registry{};
    bool called{false};

    registry.statement("Stmt", [&](const Node &, RuntimeContext &context) {
        called = true;
        context.env().define("x", Value::integer(1));
    });

    Runtime runtime{registry};
    Node root{"Stmt"};

    runtime.exec(root);

    CHECK(called);
}

TEST(Runtime, EvalUsesFreshContextEachCall) {
    RuntimeRegistry registry{};
    int calls{0};

    registry.expression("Counter", [&](const Node &, RuntimeContext &context) {
        ++calls;
        CHECK(context.env().resolve("x") == nullptr);
        context.env().define("x", Value::integer(calls));
        return Value::integer(calls);
    });

    Runtime runtime{registry};
    Node root{"Counter"};

    CHECK(runtime.eval(root).asInt() == 1);
    CHECK(runtime.eval(root).asInt() == 2);
}
