#include "../../tester.hpp"

#include "novac/assets/atomic/LiteralFeature.hpp"
#include "novac/assets/atomic/OperationFeature.hpp"
#include "novac/assets/types/TypeController.hpp"

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
    CHECK(controller.requireType(TypeId{"Widget"}).kind() == types::TypeKind::UserDefined);
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
    controller.registerSemanticRule(add, std::make_shared<BadRule>());
    std::vector<TypeId> operands{TypeId{"u8"}, TypeId{"u8"}};
    CHECK(throws([&] { (void)controller.resolveOperationDetailed(add, operands); }));
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
    controller.registerSemanticRule(unary, std::make_shared<Rule>());
    std::vector<TypeId> operands{TypeId{"u8"}};
    auto result = controller.resolveOperationDetailed(unary, operands);
    CHECK(result.ok());
    CHECK_EQ(result.resolution->conversions.size(), static_cast<std::size_t>(1));
    CHECK_EQ(result.resolution->conversions[0].conversion.rank, static_cast<std::size_t>(7));
}

} // namespace
