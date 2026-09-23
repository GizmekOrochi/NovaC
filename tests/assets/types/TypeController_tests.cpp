#include "../../tester.hpp"

#include "novac/assets/atomic/LiteralFeature.hpp"
#include "novac/assets/atomic/OperationFeature.hpp"
#include "novac/assets/types/TypeController.hpp"

#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace novac::assets;
using types::TypeId;

class TestLiteral final : public atomic::LiteralFeature {
public:
    TestLiteral(std::string id, std::string kind) : id_{std::move(id)}, kind_{std::move(kind)} {}
    atomic::LiteralInfo info() const override {
        return atomic::LiteralInfo{id_, "1.0.0", "test literal", kind_, atomic::TokenPattern::key("$int"), {}, {}};
    }
    void install(atomic::AtomicController &) const override {}
private:
    std::string id_;
    std::string kind_;
};

class TestOperation final : public atomic::OperationFeature {
public:
    TestOperation(std::string id, atomic::OperationArity arity) : id_{std::move(id)}, arity_{arity} {}
    atomic::OperationInfo info() const override {
        return atomic::OperationInfo{id_, "1.0.0", "test op", arity_, atomic::TokenPattern::text("+"), 10, novac::parser::Associativity::Left, {}, {}};
    }
    void install(atomic::AtomicController &) const override {}
private:
    std::string id_;
    atomic::OperationArity arity_;
};

struct Marker { int value; };

class CustomType final : public types::TypeDefinition {
public:
    explicit CustomType(TypeId id) : TypeDefinition{std::move(id)} {}
    int payload{42};
};

class FreezingType final : public types::TypeDefinition {
public:
    explicit FreezingType(TypeId id) : TypeDefinition{std::move(id)} {}
    bool hookCalled{false};
protected:
    void onFreeze() override { hookCalled = true; }
};

class BadRule final : public types::OperationSemanticRule {
public:
    std::optional<types::SemanticOperationResolution> resolve(
        const types::TypeController &,
        std::span<const TypeId>
    ) const override {
        types::SemanticOperationResolution out;
        out.result = TypeId{"u16"};
        out.conversions.push_back(types::SemanticConversionRequest{9, TypeId{"u16"}});
        return out;
    }
};

bool throws(const std::function<void()> &fn) {
    try { fn(); } catch (const std::exception &) { return true; }
    return false;
}

TEST(TypeController, RegistersGenericCustomTypes) {
    types::TypeController controller;
    auto custom = std::make_unique<CustomType>(TypeId{"Widget"});
    CHECK(controller.registerType(std::move(custom)) == novac::registry::RegisterStatus::Inserted);
    CHECK(controller.hasType(TypeId{"Widget"}));
    CHECK(!controller.hasPrimitive(TypeId{"Widget"}));
    CHECK(dynamic_cast<const CustomType *>(&controller.requireType(TypeId{"Widget"})) != nullptr);
}

TEST(TypeController, SeparatesSemanticAndStorageWidths) {
    types::TypeController controller;
    controller.definePrimitive("u24").bits(24).storageBits(32).alignment(4).unsignedType().commit();
    const auto &type = controller.requirePrimitive("u24");
    CHECK_EQ(type.bitWidth, static_cast<std::size_t>(24));
    CHECK_EQ(type.storageBits, static_cast<std::size_t>(32));
    CHECK_EQ(type.valueByteWidth(), static_cast<std::size_t>(3));
    CHECK_EQ(type.storageByteWidth(), static_cast<std::size_t>(4));
    CHECK_EQ(type.byteWidth(), static_cast<std::size_t>(4));
}

TEST(TypeController, AcceptsNonPowerOfTwoAlignment) {
    types::TypeController controller;
    controller.definePrimitive("packed24").bits(24).storageBits(24).alignment(3).commit();
    CHECK_EQ(controller.requirePrimitive("packed24").alignment, static_cast<std::size_t>(3));
}

TEST(TypeController, RequiresConversionTargetsToExistOrBeDeclared) {
    types::TypeController controller;
    CHECK(throws([&] {
        controller.definePrimitive("u8").bits(8)
            .conversionTo(TypeId{"u16"}, types::ConversionKind::Implicit, 1)
            .commit();
    }));

    types::TypeController forward;
    forward.declareType("u16");
    forward.definePrimitive("u8").bits(8)
        .conversionTo(TypeId{"u16"}, types::ConversionKind::Implicit, 1)
        .commit();
    forward.definePrimitive("u16").bits(16).commit();
    CHECK(forward.canImplicitlyConvert(TypeId{"u8"}, TypeId{"u16"}));
}

TEST(TypeController, FreezesCommittedExtensions) {
    types::TypeController controller;
    controller.definePrimitive("tagged").bits(8).extension<Marker>(Marker{7}).commit();
    const auto &type = controller.requirePrimitive("tagged");
    CHECK(type.extensions.frozen());
    CHECK(type.extensions.get<Marker>() != nullptr);
    CHECK_EQ(type.extensions.get<Marker>()->value, 7);

    auto copy = type.extensions;
    CHECK(throws([&] { copy.emplace<Marker>(Marker{9}); }));
}

TEST(TypeController, RejectsOperationSignaturesWithWrongArity) {
    types::TypeController controller;
    controller.definePrimitive("i32").bits(32).commit();
    TestOperation add{"add", atomic::OperationArity::Binary};
    CHECK(throws([&] {
        controller.registerOperation(add, {TypeId{"i32"}}, TypeId{"i32"});
    }));
}

TEST(TypeController, RejectsOperationSignaturesWithUnknownTypes) {
    types::TypeController controller;
    controller.definePrimitive("i32").bits(32).commit();
    TestOperation add{"add", atomic::OperationArity::Binary};
    CHECK(throws([&] {
        controller.registerOperation(add, {TypeId{"i32"}, TypeId{"Ghost"}}, TypeId{"i32"});
    }));
}

TEST(TypeController, ReportsInvalidRuntimeArity) {
    types::TypeController controller;
    controller.definePrimitive("i32").bits(32).commit();
    TestOperation add{"add", atomic::OperationArity::Binary};
    std::vector<TypeId> operands{TypeId{"i32"}};
    auto result = controller.resolveOperationDetailed(add, operands);
    CHECK(result.status == types::OperationResolutionStatus::InvalidArity);
}

TEST(TypeController, ChoosesLowestDirectImplicitConversionCost) {
    types::TypeController controller;
    controller.declareType("u16");
    controller.declareType("u32");
    controller.definePrimitive("u8").bits(8)
        .conversionTo(TypeId{"u16"}, types::ConversionKind::Implicit, 1)
        .conversionTo(TypeId{"u32"}, types::ConversionKind::Implicit, 5)
        .commit();
    controller.definePrimitive("u16").bits(16).commit();
    controller.definePrimitive("u32").bits(32).commit();

    TestOperation add{"add", atomic::OperationArity::Binary};
    controller.registerOperation(add, {TypeId{"u16"}, TypeId{"u16"}}, TypeId{"u16"});
    controller.registerOperation(add, {TypeId{"u32"}, TypeId{"u32"}}, TypeId{"u32"});

    std::vector<TypeId> operands{TypeId{"u8"}, TypeId{"u8"}};
    auto result = controller.resolveOperationDetailed(add, operands);
    CHECK(result.ok());
    CHECK(result.resolution->result == TypeId{"u16"});
    CHECK_EQ(result.resolution->conversions.size(), static_cast<std::size_t>(2));
}

TEST(TypeController, DoesNotChainImplicitConversions) {
    types::TypeController controller;
    controller.declareType("u16");
    controller.definePrimitive("u8").bits(8)
        .conversionTo(TypeId{"u16"}, types::ConversionKind::Implicit, 1).commit();
    controller.declareType("u32");
    controller.definePrimitive("u16").bits(16)
        .conversionTo(TypeId{"u32"}, types::ConversionKind::Implicit, 1).commit();
    controller.definePrimitive("u32").bits(32).commit();

    TestOperation add{"add", atomic::OperationArity::Binary};
    controller.registerOperation(add, {TypeId{"u32"}, TypeId{"u32"}}, TypeId{"u32"});
    std::vector<TypeId> operands{TypeId{"u8"}, TypeId{"u8"}};
    auto result = controller.resolveOperationDetailed(add, operands);
    CHECK(result.status == types::OperationResolutionStatus::NoMatch);
}

TEST(TypeController, ValidatesSemanticRuleConversionPlans) {
    types::TypeController controller;
    controller.declareType("u16");
    controller.definePrimitive("u8").bits(8)
        .conversionTo(TypeId{"u16"}, types::ConversionKind::Implicit, 1).commit();
    controller.definePrimitive("u16").bits(16).commit();
    TestOperation add{"add", atomic::OperationArity::Binary};
    controller.registerSemanticRule(add, std::make_unique<BadRule>());
    std::vector<TypeId> operands{TypeId{"u8"}, TypeId{"u8"}};
    const auto result = controller.resolveOperationDetailed(add, operands);
    CHECK(result.status == types::OperationResolutionStatus::InvalidConfiguration);
    CHECK(!result.diagnostic.message.empty());
}

TEST(TypeController, LiteralFeatureIdentityAvoidsNodeKindOverwrite) {
    types::TypeController controller;
    controller.definePrimitive("i8").bits(8).commit();
    controller.definePrimitive("i32").bits(32).commit();

    TestLiteral small{"small-int", "IntegerLiteral"};
    TestLiteral large{"large-int", "IntegerLiteral"};
    controller.bindLiteral(small, TypeId{"i8"});
    controller.bindLiteral(large, TypeId{"i32"});

    novac::ast::Node node{"IntegerLiteral"};
    CHECK(controller.resolveLiteral(small, node) == std::optional<TypeId>{TypeId{"i8"}});
    CHECK(controller.resolveLiteral(large, node) == std::optional<TypeId>{TypeId{"i32"}});
    CHECK(throws([&] { (void)controller.resolveLiteral(node); }));
}

TEST(TypeController, SupportsConversionsBetweenArbitraryRegisteredTypes) {
    types::TypeController controller;
    controller.registerType(std::make_unique<CustomType>(TypeId{"A"}));
    controller.registerType(std::make_unique<CustomType>(TypeId{"B"}));
    CHECK(controller.registerConversion(TypeId{"A"}, TypeId{"B"}, types::ConversionKind::Implicit, 2)
          == novac::registry::RegisterStatus::Inserted);
    CHECK(controller.canImplicitlyConvert(TypeId{"A"}, TypeId{"B"}));
    CHECK(controller.findConversion(TypeId{"A"}, TypeId{"B"}) != nullptr);
}

TEST(TypeController, ValidateRejectsUnresolvedForwardDeclarations) {
    types::TypeController controller;
    controller.declareType("Future");
    CHECK(throws([&] { controller.validate(); }));
    controller.registerType(std::make_unique<CustomType>(TypeId{"Future"}));
    controller.validate();
}

TEST(TypeController, SemanticRulesUseRegisteredConversionMetadata) {
    class Rule final : public types::OperationSemanticRule {
    public:
        std::optional<types::SemanticOperationResolution> resolve(
            const types::TypeController &,
            std::span<const TypeId>
        ) const override {
            types::SemanticOperationResolution out;
            out.result = TypeId{"u16"};
            out.conversions.push_back({0, TypeId{"u16"}});
            return out;
        }
    };

    types::TypeController controller;
    controller.declareType("u16");
    controller.definePrimitive("u8").bits(8)
        .conversionTo(TypeId{"u16"}, types::ConversionKind::Implicit, 7).commit();
    controller.definePrimitive("u16").bits(16).commit();
    TestOperation unary{"promote", atomic::OperationArity::Unary};
    controller.registerSemanticRule(unary, std::make_unique<Rule>());
    std::vector<TypeId> operands{TypeId{"u8"}};
    auto result = controller.resolveOperationDetailed(unary, operands);
    CHECK(result.ok());
    CHECK_EQ(result.resolution->conversions.size(), static_cast<std::size_t>(1));
    CHECK_EQ(result.resolution->conversions[0].conversion.rank, static_cast<std::size_t>(7));
}


TEST(TypeController, AliasesCanonicalizeWithoutCreatingConversions) {
    types::TypeController controller;
    controller.definePrimitive("u64").bits(64).commit();
    CHECK(controller.registerAlias("size_t", TypeId{"u64"}) == novac::registry::RegisterStatus::Inserted);
    CHECK(controller.hasAlias(TypeId{"size_t"}));
    CHECK(controller.hasType(TypeId{"size_t"}));
    CHECK(controller.canonical(TypeId{"size_t"}) == TypeId{"u64"});
    CHECK(controller.equivalent(TypeId{"size_t"}, TypeId{"u64"}));
    CHECK(&controller.requireType(TypeId{"size_t"}) == &controller.requireType(TypeId{"u64"}));
    CHECK(controller.findConversion(TypeId{"size_t"}, TypeId{"u64"}) == nullptr);
}

TEST(TypeController, AliasChainsResolveAndCyclesAreRejected) {
    types::TypeController controller;
    controller.definePrimitive("u32").bits(32).commit();
    controller.registerAlias("word", TypeId{"u32"});
    controller.registerAlias("index", TypeId{"word"});
    CHECK(controller.canonical(TypeId{"index"}) == TypeId{"u32"});

    types::TypeController cyclic;
    cyclic.declareType("B");
    cyclic.registerAlias("A", TypeId{"B"});
    CHECK(throws([&] { cyclic.registerAlias("B", TypeId{"A"}); }));
}

TEST(TypeController, AliasCanTargetForwardDeclarationAndValidateLater) {
    types::TypeController controller;
    controller.declareType("u64");
    controller.registerAlias("size_t", TypeId{"u64"});
    CHECK(throws([&] { controller.validate(); }));
    controller.definePrimitive("u64").bits(64).commit();
    controller.validate();
    CHECK(controller.hasType(TypeId{"size_t"}));
}

TEST(TypeController, FinalizePreventsFurtherTypeMutation) {
    types::TypeController controller;
    controller.definePrimitive("i32").bits(32).commit();
    controller.finalize();
    CHECK(controller.finalized());
    controller.finalize();
    CHECK(throws([&] { controller.definePrimitive("i64"); }));
    CHECK(throws([&] { controller.declareType("Future"); }));
    CHECK(throws([&] { controller.registerAlias("int", TypeId{"i32"}); }));
}

TEST(TypeController, ReplacementPreservesIndependentConversions) {
    types::TypeController controller{types::TypeControllerOptions{novac::registry::DuplicatePolicy::Replace}};
    controller.definePrimitive("u8").bits(8).commit();
    controller.definePrimitive("u16").bits(16).commit();
    controller.registerConversion(TypeId{"u8"}, TypeId{"u16"}, types::ConversionKind::Implicit, 3);
    controller.definePrimitive("u8").bits(8).storageBits(16).commit();
    const auto *conversion = controller.findConversion(TypeId{"u8"}, TypeId{"u16"});
    CHECK(conversion != nullptr);
    CHECK_EQ(conversion->rank, static_cast<std::size_t>(3));
}

TEST(TypeController, DuplicateConversionPoliciesAreRespected) {
    types::TypeController ignore{types::TypeControllerOptions{novac::registry::DuplicatePolicy::Ignore}};
    ignore.definePrimitive("A").bits(8).commit();
    ignore.definePrimitive("B").bits(8).commit();
    ignore.registerConversion(TypeId{"A"}, TypeId{"B"}, types::ConversionKind::Implicit, 1);
    CHECK(ignore.registerConversion(TypeId{"A"}, TypeId{"B"}, types::ConversionKind::Implicit, 9)
          == novac::registry::RegisterStatus::Ignored);
    CHECK_EQ(ignore.findConversion(TypeId{"A"}, TypeId{"B"})->rank, static_cast<std::size_t>(1));

    types::TypeController replace{types::TypeControllerOptions{novac::registry::DuplicatePolicy::Replace}};
    replace.definePrimitive("A").bits(8).commit();
    replace.definePrimitive("B").bits(8).commit();
    replace.registerConversion(TypeId{"A"}, TypeId{"B"}, types::ConversionKind::Implicit, 1);
    CHECK(replace.registerConversion(TypeId{"A"}, TypeId{"B"}, types::ConversionKind::Implicit, 9)
          == novac::registry::RegisterStatus::Replaced);
    CHECK_EQ(replace.findConversion(TypeId{"A"}, TypeId{"B"})->rank, static_cast<std::size_t>(9));
}

TEST(TypeController, EqualCostOverloadsReportCandidates) {
    types::TypeController controller;
    controller.definePrimitive("u8").bits(8).commit();
    controller.definePrimitive("u16").bits(16).commit();
    controller.definePrimitive("u32").bits(32).commit();
    controller.registerConversion(TypeId{"u8"}, TypeId{"u16"}, types::ConversionKind::Implicit, 1);
    controller.registerConversion(TypeId{"u8"}, TypeId{"u32"}, types::ConversionKind::Implicit, 1);

    TestOperation add{"add", atomic::OperationArity::Binary};
    controller.registerOperation(add, {TypeId{"u16"}, TypeId{"u16"}}, TypeId{"u16"});
    controller.registerOperation(add, {TypeId{"u32"}, TypeId{"u32"}}, TypeId{"u32"});
    const std::vector<TypeId> operands{TypeId{"u8"}, TypeId{"u8"}};
    const auto result = controller.resolveOperationDetailed(add, operands);
    CHECK(result.status == types::OperationResolutionStatus::Ambiguous);
    CHECK_EQ(result.diagnostic.candidates.size(), static_cast<std::size_t>(2));
    CHECK(!result.diagnostic.message.empty());
}

TEST(TypeController, SemanticRulePriorityTiesAreAmbiguous) {
    class Rule final : public types::OperationSemanticRule {
    public:
        explicit Rule(TypeId result) : result_{std::move(result)} {}
        int priority() const noexcept override { return 5; }
        std::optional<types::SemanticOperationResolution> resolve(
            const types::TypeController &, std::span<const TypeId>
        ) const override {
            types::SemanticOperationResolution out;
            out.result = result_;
            return out;
        }
    private:
        TypeId result_;
    };

    types::TypeController controller;
    controller.definePrimitive("i32").bits(32).commit();
    controller.definePrimitive("i64").bits(64).commit();
    TestOperation unary{"promote", atomic::OperationArity::Unary};
    controller.registerSemanticRule(unary, std::make_unique<Rule>(TypeId{"i32"}));
    controller.registerSemanticRule(unary, std::make_unique<Rule>(TypeId{"i64"}));
    const std::vector<TypeId> operands{TypeId{"i32"}};
    const auto result = controller.resolveOperationDetailed(unary, operands);
    CHECK(result.status == types::OperationResolutionStatus::Ambiguous);
}

TEST(TypeController, LiteralResolverMayReturnAliasAndIsCanonicalized) {
    types::TypeController controller;
    controller.definePrimitive("i32").bits(32).commit();
    controller.registerAlias("int", TypeId{"i32"});
    TestLiteral literal{"int-literal", "IntegerLiteral"};
    controller.bindLiteral(literal, [](const novac::ast::Node &) -> std::optional<TypeId> {
        return TypeId{"int"};
    });
    novac::ast::Node node{"IntegerLiteral"};
    CHECK(controller.resolveLiteral(literal, node) == std::optional<TypeId>{TypeId{"i32"}});
}

TEST(TypeController, LiteralResolverUnknownTypeIsRejected) {
    types::TypeController controller;
    controller.definePrimitive("i32").bits(32).commit();
    TestLiteral literal{"bad-literal", "IntegerLiteral"};
    controller.bindLiteral(literal, [](const novac::ast::Node &) -> std::optional<TypeId> {
        return TypeId{"Ghost"};
    });
    novac::ast::Node node{"IntegerLiteral"};
    CHECK(throws([&] { (void)controller.resolveLiteral(literal, node); }));
}

TEST(TypeController, OverflowingConversionCostsDoNotBecomeBetterCandidates) {
    types::TypeController controller;
    controller.definePrimitive("A").bits(8).commit();
    controller.definePrimitive("B").bits(8).commit();
    controller.definePrimitive("C").bits(8).commit();
    controller.registerConversion(
        TypeId{"A"}, TypeId{"B"}, types::ConversionKind::Implicit,
        std::numeric_limits<std::size_t>::max()
    );
    TestOperation add{"add", atomic::OperationArity::Binary};
    controller.registerOperation(add, {TypeId{"B"}, TypeId{"B"}}, TypeId{"B"});
    controller.registerOperation(add, {TypeId{"C"}, TypeId{"C"}}, TypeId{"C"});
    controller.registerConversion(TypeId{"A"}, TypeId{"C"}, types::ConversionKind::Implicit, 10);
    const std::vector<TypeId> operands{TypeId{"A"}, TypeId{"A"}};
    const auto result = controller.resolveOperationDetailed(add, operands);
    CHECK(result.ok());
    CHECK(result.resolution->result == TypeId{"C"});
}

TEST(TypeController, DiagnosticExplainsInvalidArityAndUnknownOperand) {
    types::TypeController controller;
    controller.definePrimitive("i32").bits(32).commit();
    TestOperation add{"add", atomic::OperationArity::Binary};
    const std::vector<TypeId> one{TypeId{"i32"}};
    const auto arity = controller.resolveOperationDetailed(add, one);
    CHECK(arity.status == types::OperationResolutionStatus::InvalidArity);
    CHECK(!arity.diagnostic.message.empty());

    const std::vector<TypeId> unknown{TypeId{"i32"}, TypeId{"Ghost"}};
    const auto operand = controller.resolveOperationDetailed(add, unknown);
    CHECK(operand.status == types::OperationResolutionStatus::UnknownOperand);
    CHECK(!operand.diagnostic.message.empty());
}


TEST(TypeController, CustomTypeFreezeHookCannotBypassBaseFreeze) {
    types::TypeController controller;
    auto custom = std::make_unique<FreezingType>(TypeId{"Frozen"});
    custom->extensions.emplace<Marker>(Marker{12});
    controller.registerType(std::move(custom));
    const auto &stored = dynamic_cast<const FreezingType &>(controller.requireType(TypeId{"Frozen"}));
    CHECK(stored.hookCalled);
    CHECK(stored.extensions.frozen());
    CHECK_EQ(stored.extensions.get<Marker>()->value, 12);
}

} // namespace
