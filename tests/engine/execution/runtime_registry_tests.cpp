#include "../../tester.hpp"
#include "novac/engine/execution/Runtime.hpp"
#include "novac/engine/syntax/Node.hpp"

#include <stdexcept>
#include <string>

namespace {

using novac::ast::Node;
using novac::ast::NodePtr;
using novac::registry::DuplicatePolicy;
using novac::registry::RegisterStatus;
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

NodePtr makeNode(std::string kind) {
    return Node::make(std::move(kind));
}

Node binaryNode(std::string op) {
    Node node{"binary"};
    node.set("op", std::move(op));
    return node;
}

} // namespace

TEST(RuntimeRegistry, HasDefaultBinaryNodeKind) {
    RuntimeRegistry registry{};

    CHECK(registry.binaryNodeKind() == std::string{"binary"});
}

TEST(RuntimeRegistry, SetsBinaryNodeKind) {
    RuntimeRegistry registry{};

    registry.setBinaryNodeKind("BinaryExpression");

    CHECK(registry.binaryNodeKind() == "BinaryExpression");
}

TEST(RuntimeRegistry, RejectsEmptyBinaryNodeKind) {
    RuntimeRegistry registry{};

    CHECK(throwsRuntimeError([&]() { registry.setBinaryNodeKind(""); }));
}

TEST(RuntimeRegistry, RegistersAndEvaluatesExpression) {
    RuntimeRegistry registry{};

    CHECK(registry.expression("Literal", [](const Node &, RuntimeContext &) {
        return Value::integer(42);
    }) == RegisterStatus::Inserted);

    RuntimeContext context{registry};
    Node node{"Literal"};

    CHECK(registry.eval(node, context).asInt() == 42);
}

TEST(RuntimeRegistry, RegistersExpressionWithTypedKind) {
    RuntimeRegistry registry{};
    novac::ids::NodeKind kind{"Literal"};

    CHECK(registry.expression(kind, [](const Node &, RuntimeContext &) {
        return Value::integer(10);
    }) == RegisterStatus::Inserted);

    RuntimeContext context{registry};
    Node node{"Literal"};

    CHECK(registry.eval(node, context).asInt() == 10);
}

TEST(RuntimeRegistry, RejectsInvalidExpressionRegistration) {
    RuntimeRegistry registry{};

    CHECK(throwsRuntimeError([&]() { registry.expression("", [](const Node &, RuntimeContext &) { return Value::voidValue(); }); }));
    CHECK(throwsRuntimeError([&]() { registry.expression("Literal", {}); }));
}

TEST(RuntimeRegistry, RejectsMissingExpressionHandler) {
    RuntimeRegistry registry{};
    RuntimeContext context{registry};
    Node node{"Missing"};

    CHECK(throwsRuntimeError([&]() {
        static_cast<void>(registry.eval(node, context));
    }));
}

TEST(RuntimeRegistry, RegistersAndExecutesStatement) {
    RuntimeRegistry registry{};
    bool called{false};

    CHECK(registry.statement("Stmt", [&](const Node &, RuntimeContext &) {
        called = true;
    }) == RegisterStatus::Inserted);

    RuntimeContext context{registry};
    Node node{"Stmt"};

    registry.exec(node, context);

    CHECK(called);
}

TEST(RuntimeRegistry, RegistersStatementWithTypedKind) {
    RuntimeRegistry registry{};
    bool called{false};
    novac::ids::NodeKind kind{"Stmt"};

    CHECK(registry.statement(kind, [&](const Node &, RuntimeContext &) {
        called = true;
    }) == RegisterStatus::Inserted);

    RuntimeContext context{registry};
    Node node{"Stmt"};

    registry.exec(node, context);

    CHECK(called);
}

TEST(RuntimeRegistry, RejectsInvalidStatementRegistration) {
    RuntimeRegistry registry{};

    CHECK(throwsRuntimeError([&]() { registry.statement("", [](const Node &, RuntimeContext &) {}); }));
    CHECK(throwsRuntimeError([&]() { registry.statement("Stmt", {}); }));
}

TEST(RuntimeRegistry, RejectsMissingStatementHandler) {
    RuntimeRegistry registry{};
    RuntimeContext context{registry};
    Node node{"Missing"};

    CHECK(throwsRuntimeError([&]() { registry.exec(node, context); }));
}

TEST(RuntimeRegistry, RegistersAndExecutesDeclaration) {
    RuntimeRegistry registry{};
    bool called{false};

    CHECK(registry.declaration("Decl", [&](const NodePtr &node, RuntimeContext &) {
        called = node != nullptr && node->kind() == "Decl";
    }) == RegisterStatus::Inserted);

    RuntimeContext context{registry};
    NodePtr node{makeNode("Decl")};

    registry.declare(node, context);

    CHECK(called);
}

TEST(RuntimeRegistry, RegistersDeclarationWithTypedKind) {
    RuntimeRegistry registry{};
    bool called{false};
    novac::ids::NodeKind kind{"Decl"};

    CHECK(registry.declaration(kind, [&](const NodePtr &, RuntimeContext &) {
        called = true;
    }) == RegisterStatus::Inserted);

    RuntimeContext context{registry};
    NodePtr node{makeNode("Decl")};

    registry.declare(node, context);

    CHECK(called);
}

TEST(RuntimeRegistry, TryDeclareReturnsTrueWhenHandled) {
    RuntimeRegistry registry{};

    registry.declaration("Decl", [](const NodePtr &, RuntimeContext &) {});

    RuntimeContext context{registry};
    NodePtr node{makeNode("Decl")};

    CHECK(registry.tryDeclare(node, context));
}

TEST(RuntimeRegistry, TryDeclareReturnsFalseForNullOrMissingNode) {
    RuntimeRegistry registry{};
    RuntimeContext context{registry};

    CHECK(!registry.tryDeclare(nullptr, context));

    NodePtr node{makeNode("Missing")};
    CHECK(!registry.tryDeclare(node, context));
}

TEST(RuntimeRegistry, RejectsInvalidDeclarationRegistration) {
    RuntimeRegistry registry{};

    CHECK(throwsRuntimeError([&]() {
        registry.declaration("", [](const NodePtr &, RuntimeContext &) {});
    }));

    CHECK(throwsRuntimeError([&]() {
        registry.declaration("Decl", {});
    }));
}

TEST(RuntimeRegistry, RejectsInvalidDeclarationExecution) {
    RuntimeRegistry registry{};
    RuntimeContext context{registry};

    CHECK(throwsRuntimeError([&]() {
        registry.declare(nullptr, context);
    }));

    NodePtr node{makeNode("Missing")};

    CHECK(throwsRuntimeError([&]() {
        registry.declare(node, context);
    }));
}

TEST(RuntimeRegistry, RegistersBinaryOperatorAndEvaluatesThroughDispatcher) {
    RuntimeRegistry registry{};

    CHECK(registry.binaryOperator("+", [](const Node &, RuntimeContext &) {
        return Value::integer(3);
    }) == RegisterStatus::Inserted);

    RuntimeContext context{registry};
    Node node{binaryNode("+")};

    CHECK(registry.eval(node, context).asInt() == 3);
}

TEST(RuntimeRegistry, RegistersBinaryOperatorWithTypedOperation) {
    RuntimeRegistry registry{};
    novac::ids::Operation op{"+"};

    CHECK(registry.binaryOperator(op, [](const Node &, RuntimeContext &) {
        return Value::integer(5);
    }) == RegisterStatus::Inserted);

    RuntimeContext context{registry};
    Node node{binaryNode("+")};

    CHECK(registry.eval(node, context).asInt() == 5);
}

TEST(RuntimeRegistry, RejectsInvalidBinaryOperatorRegistration) {
    RuntimeRegistry registry{};

    CHECK(throwsRuntimeError([&]() {
        registry.binaryOperator("", [](const Node &, RuntimeContext &) { return Value::voidValue(); });
    }));

    CHECK(throwsRuntimeError([&]() {
        registry.binaryOperator("+", {});
    }));
}

TEST(RuntimeRegistry, RejectsMissingBinaryOperatorHandler) {
    RuntimeRegistry registry{};
    registry.binaryOperator("+", [](const Node &, RuntimeContext &) {
        return Value::integer(1);
    });

    RuntimeContext context{registry};
    Node node{binaryNode("-")};

    CHECK(throwsRuntimeError([&]() {
        static_cast<void>(registry.eval(node, context));
    }));
}

TEST(RuntimeRegistry, RejectsChangingBinaryNodeKindAfterDispatcherInstall) {
    RuntimeRegistry registry{};

    registry.binaryOperator("+", [](const Node &, RuntimeContext &) {
        return Value::integer(1);
    });

    CHECK(throwsRuntimeError([&]() {
        registry.setBinaryNodeKind("OtherBinary");
    }));
}

TEST(RuntimeRegistry, DuplicateExpressionUsesPolicy) {
    RuntimeRegistry registry{DuplicatePolicy::Ignore};

    CHECK(registry.expression("Literal", [](const Node &, RuntimeContext &) {
        return Value::integer(1);
    }) == RegisterStatus::Inserted);

    CHECK(registry.expression("Literal", [](const Node &, RuntimeContext &) {
        return Value::integer(2);
    }) == RegisterStatus::Ignored);
}
