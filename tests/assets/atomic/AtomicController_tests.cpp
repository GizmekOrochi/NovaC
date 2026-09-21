#include "../../tester.hpp"
#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/assets/atomic/literals/IntegerLiteralAtomic.hpp"
#include "novac/assets/atomic/literals/BooleanLiteralAtomic.hpp"
#include "novac/assets/atomic/operations/NumericOperations.hpp"
#include "novac/assets/atomic/operations/ComparisonOperations.hpp"

#include <limits>

namespace {

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

using novac::assets::atomic::AtomicController;
using novac::assets::atomic::AtomicControllerOptions;
using novac::assets::atomic::LiteralInfo;
using novac::assets::atomic::OperationInfo;
using novac::assets::atomic::literals::IntegerLiteralAtomic;
using novac::assets::atomic::literals::BooleanLiteralAtomic;
using novac::assets::atomic::operations::AddOperationAtomic;
using novac::assets::atomic::operations::LessOperationAtomic;
using novac::assets::atomic::operations::DivideOperationAtomic;
using novac::assets::atomic::operations::ModuloOperationAtomic;
using novac::controllers::EngineController;

class ThrowingLiteralFeature final : public novac::assets::atomic::LiteralFeature {
public:
    novac::assets::atomic::LiteralInfo info() const override {
        return {
            "test.literal.transaction",
            "0.1.0",
            "Literal that fails after mutating the engine",
            "TransactionalLiteral",
            novac::assets::atomic::TokenPattern::keywordText("txlit"),
            {"test.literal.transaction"},
            {}
        };
    }

    void install(AtomicController &controller) const override {
        controller.registerPattern(info().pattern);
        controller.engine().node({
            .kind = "TransactionalLiteral",
            .traits = {"expr"},
            .doc = "Temporary transactional literal"
        });
        throw std::runtime_error("literal install failed");
    }
};

class ThrowingOperationFeature final : public novac::assets::atomic::OperationFeature {
public:
    novac::assets::atomic::OperationInfo info() const override {
        return {
            "test.operation.transaction",
            "0.1.0",
            "Operation that fails after mutating the engine and controller",
            novac::assets::atomic::OperationArity::Unary,
            novac::assets::atomic::TokenPattern::text("~"),
            80,
            novac::parser::Associativity::Right,
            {"test.operation.transaction"},
            {"expression.atom"}
        };
    }

    void install(AtomicController &controller) const override {
        controller.registerPattern(info().pattern);
        controller.ensureUnaryExpressionNode();
        controller.registerUnaryOperation(
            info().id,
            [](const novac::ast::Node &, novac::runtime::RuntimeContext &) {
                return novac::runtime::Value::integer(0);
            });
        throw std::runtime_error("operation install failed");
    }
};

TEST(AtomicController, DefaultConstruction) {
    EngineController engine;
    AtomicController controller{engine};
    CHECK(&controller.engine() == &engine);
}

TEST(AtomicController, CustomOptionsConstruction) {
    EngineController engine;
    AtomicControllerOptions options;
    options.expressionDomain = "custom_expr";
    options.binaryNodeKind = "CustomBinary";
    options.unaryNodeKind = "CustomUnary";

    AtomicController controller{engine, options};
    CHECK(controller.expressionDomain() == "custom_expr");
    CHECK(controller.binaryNodeKind() == "CustomBinary");
    CHECK(controller.unaryNodeKind() == "CustomUnary");
}

TEST(AtomicController, EmptyExpressionDomainThrows) {
    EngineController engine;
    AtomicControllerOptions options;
    options.expressionDomain = "";
    CHECK(throwsRuntimeError([&]() { AtomicController controller{engine, options}; }));
}

TEST(AtomicController, EmptyBinaryNodeKindThrows) {
    EngineController engine;
    AtomicControllerOptions options;
    options.binaryNodeKind = "";
    CHECK(throwsRuntimeError([&]() { AtomicController controller{engine, options}; }));
}

TEST(AtomicController, EmptyUnaryNodeKindThrows) {
    EngineController engine;
    AtomicControllerOptions options;
    options.unaryNodeKind = "";
    CHECK(throwsRuntimeError([&]() {
        AtomicController controller{engine, options};
    }));
}

TEST(AtomicController, FailedLiteralInstallRollsBackEngineAndController) {
    EngineController engine;
    AtomicController controller{engine};
    ThrowingLiteralFeature feature;

    CHECK(throwsRuntimeError([&]() { controller.use(feature); }));

    CHECK(!controller.hasLiteral("test.literal.transaction"));
    CHECK(controller.literals().empty());
    CHECK(!controller.hasCapability("test.literal.transaction"));
    CHECK(!engine.lexer().isKeyword("txlit"));
    CHECK(engine.nodes().find("TransactionalLiteral") == nullptr);

    controller.integer();
    CHECK(controller.hasLiteral("core.literal.integer"));
}

TEST(AtomicController, FailedOperationInstallRollsBackEngineAndController) {
    EngineController engine;
    AtomicController controller{engine};
    ThrowingOperationFeature feature;

    controller.integer();
    const std::size_t literalCount{controller.literals().size()};

    CHECK(throwsRuntimeError([&]() { controller.use(feature); }));

    CHECK(!controller.hasOperation("test.operation.transaction"));
    CHECK(controller.operations().empty());
    CHECK(controller.literals().size() == literalCount);
    CHECK(!controller.hasCapability("test.operation.transaction"));
    CHECK(engine.lexer().symbols().empty());
    CHECK(engine.nodes().find(controller.unaryNodeKind()) == nullptr);

    controller.registerUnaryOperation(
        "test.operation.transaction",
        [](const novac::ast::Node &, novac::runtime::RuntimeContext &) {
            return novac::runtime::Value::integer(1);
        });
}

TEST(AtomicController, UseLiteralFeature) {
    EngineController engine;
    AtomicController controller{engine};
    IntegerLiteralAtomic feature;

    controller.use(feature);
    CHECK(controller.hasLiteral("core.literal.integer"));
    CHECK(controller.literals().size() == 1);
}

TEST(AtomicController, LiteralProvidesCapabilities) {
    EngineController engine;
    AtomicController controller{engine};

    CHECK(!controller.hasCapability("expression.atom"));
    controller.integer();

    CHECK(controller.hasCapability("literal.integer"));
    CHECK(controller.hasCapability("expression.atom"));
}

TEST(AtomicController, MissingOperationCapabilityIsRejected) {
    EngineController engine;
    AtomicController controller{engine};

    CHECK(throwsRuntimeError([&]() {
        controller.use(AddOperationAtomic{});
    }));
    CHECK(!controller.hasOperation("core.op.add"));
}

TEST(AtomicController, OperationRequirementUsesCapability) {
    EngineController engine;
    AtomicController controller{engine};

    controller.integer();
    controller.use(AddOperationAtomic{});

    CHECK(controller.hasOperation("core.op.add"));
    CHECK(controller.hasCapability("operation.numeric"));
    CHECK(controller.hasCapability("operation.binary"));
}

TEST(AtomicController, UseOperationFeature) {
    EngineController engine;
    AtomicController controller{engine};
    AddOperationAtomic feature;

    controller.integer();
    controller.use(feature);
    CHECK(controller.hasOperation("core.op.add"));
    CHECK(controller.operations().size() == 1);
}

TEST(AtomicController, TemporaryNumericOperationRemainsUsable) {
    EngineController engine;
    AtomicController controller{engine};

    controller.integer();
    controller.use(AddOperationAtomic{});

    const auto expression{engine.parse("1 + 2")};
    const auto result{engine.eval(*expression)};

    CHECK(result.asInt() == 3);
}

TEST(AtomicController, TemporaryComparisonOperationRemainsUsable) {
    EngineController engine;
    AtomicController controller{engine};

    controller.integer();
    controller.use(LessOperationAtomic{});

    const auto expression{engine.parse("1 < 2")};
    const auto result{engine.eval(*expression)};

    CHECK(result.truthy());
}

TEST(AtomicController, IntegerAdditionOverflowThrowsAtRuntime) {
    EngineController engine;
    AtomicController controller{engine};

    controller.integer();
    controller.add();

    const std::string source{
        std::to_string(std::numeric_limits<int>::max()) + " + 1"};
    const auto expression{engine.parse(source)};

    CHECK(throwsRuntimeError([&]() {
        static_cast<void>(engine.eval(*expression));
    }));
}


TEST(AtomicController, IntegerDivisionByZeroThrowsAtRuntime) {
    EngineController engine;
    AtomicController controller{engine};

    controller.integer();
    controller.use(DivideOperationAtomic{});

    const auto expression{engine.parse("1 / 0")};

    CHECK(throwsRuntimeError([&]() {
        static_cast<void>(engine.eval(*expression));
    }));
}

TEST(AtomicController, IntegerModuloByZeroThrowsAtRuntime) {
    EngineController engine;
    AtomicController controller{engine};

    controller.integer();
    controller.use(ModuloOperationAtomic{});

    const auto expression{engine.parse("1 % 0")};

    CHECK(throwsRuntimeError([&]() {
        static_cast<void>(engine.eval(*expression));
    }));
}

TEST(AtomicController, IntegerDivisionMinByNegativeOneThrowsAtRuntime) {
    EngineController engine;
    AtomicController controller{engine};

    controller.integer();
    controller.use(DivideOperationAtomic{});

    const auto left{engine.makeNode("IntegerLiteral")};
    left->set("value", std::numeric_limits<int>::min());

    const auto right{engine.makeNode("IntegerLiteral")};
    right->set("value", -1);

    const auto expression{engine.makeNode(controller.binaryNodeKind())};
    expression->set("op", std::string{"core.op.div"});
    expression->set("left", left);
    expression->set("right", right);

    CHECK(throwsRuntimeError([&]() {
        static_cast<void>(engine.eval(*expression));
    }));
}

TEST(AtomicController, IntegerModuloMinByNegativeOneReturnsZero) {
    EngineController engine;
    AtomicController controller{engine};

    controller.integer();
    controller.use(ModuloOperationAtomic{});

    const auto left{engine.makeNode("IntegerLiteral")};
    left->set("value", std::numeric_limits<int>::min());

    const auto right{engine.makeNode("IntegerLiteral")};
    right->set("value", -1);

    const auto expression{engine.makeNode(controller.binaryNodeKind())};
    expression->set("op", std::string{"core.op.mod"});
    expression->set("left", left);
    expression->set("right", right);

    CHECK(engine.eval(*expression).asInt() == 0);
}

TEST(AtomicController, IntegerRegistration) {
    EngineController engine;
    AtomicController controller{engine};
    controller.integer();
    CHECK(controller.hasLiteral("core.literal.integer"));
}

TEST(AtomicController, FloatingRegistration) {
    EngineController engine;
    AtomicController controller{engine};
    controller.floating();
    CHECK(controller.hasLiteral("core.literal.float"));
}

TEST(AtomicController, BooleanRegistration) {
    EngineController engine;
    AtomicController controller{engine};
    controller.boolean();
    CHECK(controller.hasLiteral("core.literal.boolean"));
}

TEST(AtomicController, StringLiteralRegistration) {
    EngineController engine;
    AtomicController controller{engine};
    controller.stringLiteral();
    CHECK(controller.hasLiteral("core.literal.string"));
}

TEST(AtomicController, AddOperationRegistration) {
    EngineController engine;
    AtomicController controller{engine};
    controller.integer();
    controller.add();
    CHECK(controller.hasOperation("core.op.add"));
}

TEST(AtomicController, SubtractOperationRegistration) {
    EngineController engine;
    AtomicController controller{engine};
    controller.integer();
    controller.subtract();
    CHECK(controller.hasOperation("core.op.sub"));
}

TEST(AtomicController, StandardLiterals) {
    EngineController engine;
    AtomicController controller{engine};
    controller.standardLiterals();
    CHECK(controller.hasLiteral("core.literal.integer"));
    CHECK(controller.hasLiteral("core.literal.float"));
    CHECK(controller.hasLiteral("core.literal.string"));
    CHECK(controller.hasLiteral("core.literal.boolean"));
    CHECK(controller.literals().size() == 4);
}

TEST(AtomicController, StandardNumericOperations) {
    EngineController engine;
    AtomicController controller{engine};
    controller.integer();
    controller.standardNumericOperations();
    CHECK(controller.hasOperation("core.op.add"));
    CHECK(controller.hasOperation("core.op.sub"));
    CHECK(controller.hasOperation("core.op.mul"));
    CHECK(controller.hasOperation("core.op.div"));
    CHECK(controller.hasOperation("core.op.mod"));
    CHECK(controller.hasOperation("core.op.neg"));
    CHECK(controller.operations().size() == 6);
}

TEST(AtomicController, StandardComparisonOperations) {
    EngineController engine;
    AtomicController controller{engine};
    controller.integer();
    controller.standardComparisonOperations();
    CHECK(controller.hasOperation("core.op.eq"));
    CHECK(controller.hasOperation("core.op.neq"));
    CHECK(controller.hasOperation("core.op.lt"));
    CHECK(controller.hasOperation("core.op.lte"));
    CHECK(controller.hasOperation("core.op.gt"));
    CHECK(controller.hasOperation("core.op.gte"));
    CHECK(controller.operations().size() == 6);
}

TEST(AtomicController, StandardLogicalOperations) {
    EngineController engine;
    AtomicController controller{engine};
    controller.integer();
    controller.standardLogicalOperations();
    CHECK(controller.hasOperation("core.op.logical.and"));
    CHECK(controller.hasOperation("core.op.logical.or"));
    CHECK(controller.hasOperation("core.op.logical.not"));
    CHECK(controller.operations().size() == 3);
}

TEST(AtomicController, StandardCore) {
    EngineController engine;
    AtomicController controller{engine};
    controller.standardCore();
    CHECK(controller.literals().size() == 4);
    CHECK(controller.operations().size() == 15); 
}

TEST(AtomicController, StandardOperations) {
    EngineController engine;
    AtomicController controller{engine};
    controller.integer();
    controller.standardOperations();
    CHECK(controller.operations().size() == 15);
}

TEST(AtomicController, DuplicateLiteralThrows) {
    EngineController engine;
    AtomicController controller{engine};
    IntegerLiteralAtomic feature1;
    IntegerLiteralAtomic feature2;

    controller.use(feature1);
    CHECK(throwsRuntimeError([&]() { controller.use(feature2); }));
}

TEST(AtomicController, DuplicateOperationThrows) {
    EngineController engine;
    AtomicController controller{engine};
    AddOperationAtomic feature1;
    AddOperationAtomic feature2;

    controller.integer();
    controller.use(feature1);
    CHECK(throwsRuntimeError([&]() { controller.use(feature2); }));
}

TEST(AtomicController, OwnLiteralFeature) {
    EngineController engine;
    AtomicController controller{engine};
    controller.own(std::make_unique<IntegerLiteralAtomic>());
    CHECK(controller.hasLiteral("core.literal.integer"));
}

TEST(AtomicController, OwnOperationFeature) {
    EngineController engine;
    AtomicController controller{engine};
    controller.integer();
    controller.own(std::make_unique<AddOperationAtomic>());
    CHECK(controller.hasOperation("core.op.add"));
}

TEST(AtomicController, OwnNullLiteralFeatureThrows) {
    EngineController engine;
    AtomicController controller{engine};
    CHECK(throwsRuntimeError([&]() { controller.own(std::unique_ptr<IntegerLiteralAtomic>(nullptr)); }));
}

TEST(AtomicController, OwnNullOperationFeatureThrows) {
    EngineController engine;
    AtomicController controller{engine};
    CHECK(throwsRuntimeError([&]() { controller.own(std::unique_ptr<AddOperationAtomic>(nullptr)); }));
}

TEST(AtomicController, IntegerWithSuffix) {
    EngineController engine;
    AtomicController controller{engine};
    controller.integer("MyInt", {"i32", "i64"});
    CHECK(controller.hasLiteral("core.literal.integer"));
    CHECK(controller.literals()[0].nodeKind == "MyInt");
}

TEST(AtomicController, CustomBooleanTokens) {
    EngineController engine;
    AtomicController controller{engine};
    controller.boolean("MyBool", "oui", "non");
    CHECK(controller.hasLiteral("core.literal.boolean"));
    CHECK(controller.literals()[0].nodeKind == "MyBool");
}

TEST(AtomicController, CustomOperationToken) {
    EngineController engine;
    AtomicController controller{engine};
    controller.integer();
    controller.add("plus");
    CHECK(controller.hasOperation("core.op.add"));
}

TEST(AtomicController, RegisterUnaryOperationWithEmptyIdThrows) {
    EngineController engine;
    AtomicController controller{engine};
    CHECK(throwsRuntimeError([&]() { controller.registerUnaryOperation("", [](const auto&, const auto&) { return novac::runtime::Value::integer(0); }); }));
}

TEST(AtomicController, RegisterUnaryOperationWithNullHandlerThrows) {
    EngineController engine;
    AtomicController controller{engine};
    CHECK(throwsRuntimeError([&]() { controller.registerUnaryOperation("test", nullptr); }));
}

} // namespace