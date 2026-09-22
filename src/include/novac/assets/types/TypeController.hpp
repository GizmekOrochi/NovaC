#pragma once

#include "novac/assets/types/model/Type.hpp"
#include "novac/assets/types/semantics/TypeSemantics.hpp"
#include "novac/engine/foundation/registry/Registry.hpp"

#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace novac::assets::atomic {
class LiteralFeature;
struct LiteralInfo;
class OperationFeature;
struct OperationInfo;
} // namespace novac::assets::atomic

namespace novac::assets::types {

struct TypeControllerOptions {
    registry::DuplicatePolicy duplicatePolicy{registry::DuplicatePolicy::Error};
};

class TypeController final {
public:
    class PrimitiveBuilder final {
    public:
        PrimitiveBuilder(TypeController &controller, TypeId id);

        PrimitiveBuilder &bits(std::size_t bitWidth);
        PrimitiveBuilder &storageBits(std::size_t storageBits);
        PrimitiveBuilder &alignment(std::size_t alignmentBytes);
        PrimitiveBuilder &signedness(PrimitiveSignedness value);
        PrimitiveBuilder &signedType();
        PrimitiveBuilder &unsignedType();
        PrimitiveBuilder &signless();

        template <typename Representation, typename... Args>
        PrimitiveBuilder &representation(Args &&...args) {
            static_assert(
                std::is_base_of_v<PrimitiveRepresentation, Representation>,
                "Representation must derive from PrimitiveRepresentation"
            );
            type_.representation = std::make_shared<Representation>(std::forward<Args>(args)...);
            return *this;
        }

        template <typename Extension, typename... Args>
        PrimitiveBuilder &extension(Args &&...args) {
            type_.extensions.emplace<Extension>(std::forward<Args>(args)...);
            return *this;
        }

        PrimitiveBuilder &conversionTo(
            TypeId target,
            ConversionKind kind = ConversionKind::Explicit,
            std::size_t rank = 1
        );

        template <typename Extension, typename... Args>
        PrimitiveBuilder &conversionExtension(const TypeId &target, Args &&...args) {
            auto it = pendingConversions_.find(target);
            if (it == pendingConversions_.end()) {
                throw std::runtime_error(
                    "TypeController::PrimitiveBuilder::conversionExtension: conversion must be declared first"
                );
            }
            it->second.extensions.emplace<Extension>(std::forward<Args>(args)...);
            return *this;
        }

        const PrimitiveType &definition() const noexcept { return type_; }
        registry::RegisterStatus commit();

    private:
        TypeController *controller_{nullptr};
        PrimitiveType type_{};
        std::unordered_map<TypeId, TypeConversion, TypeIdHash> pendingConversions_{};
    };

    explicit TypeController(TypeControllerOptions options = {});

    // Generic type registry -------------------------------------------------
    void declareType(TypeId id);
    void declareType(std::string name) { declareType(TypeId{std::move(name)}); }
    bool isDeclared(const TypeId &id) const noexcept;

    registry::RegisterStatus registerType(std::unique_ptr<TypeDefinition> type);
    bool hasType(const TypeId &id) const noexcept;
    bool hasType(const std::string &name) const noexcept { return hasType(TypeId{name}); }
    const TypeDefinition *findType(const TypeId &id) const noexcept;
    const TypeDefinition &requireType(const TypeId &id) const;
    const std::vector<TypeId> &typeIds() const noexcept;
    std::size_t typeCount() const noexcept;

    // Primitive helpers -----------------------------------------------------
    PrimitiveBuilder definePrimitive(TypeId id);
    PrimitiveBuilder definePrimitive(std::string name) { return definePrimitive(TypeId{std::move(name)}); }
    registry::RegisterStatus registerPrimitive(PrimitiveType type);

    bool hasPrimitive(const TypeId &id) const noexcept;
    bool hasPrimitive(const std::string &name) const noexcept { return hasPrimitive(TypeId{name}); }
    const PrimitiveType *findPrimitive(const TypeId &id) const noexcept;
    const PrimitiveType &requirePrimitive(const TypeId &id) const;
    const PrimitiveType &requirePrimitive(const std::string &name) const { return requirePrimitive(TypeId{name}); }
    std::vector<TypeId> primitiveIds() const;
    std::size_t primitiveCount() const noexcept;

    registry::RegisterStatus registerConversion(
        TypeId source,
        TypeId target,
        ConversionKind kind = ConversionKind::Explicit,
        std::size_t rank = 1,
        ExtensionSet extensions = {}
    );
    const TypeConversion *findConversion(const TypeId &source, const TypeId &target) const noexcept;
    bool canImplicitlyConvert(const TypeId &source, const TypeId &target) const noexcept;

    /** Validate unresolved forward declarations and cross-type references. */
    void validate() const;

    // Literal semantics -----------------------------------------------------
    registry::RegisterStatus bindLiteral(const atomic::LiteralFeature &literal, TypeId type);
    registry::RegisterStatus bindLiteral(const atomic::LiteralInfo &literal, TypeId type);
    registry::RegisterStatus bindLiteral(const atomic::LiteralFeature &literal, LiteralTypeResolver resolver);
    registry::RegisterStatus bindLiteral(const atomic::LiteralInfo &literal, LiteralTypeResolver resolver);

    std::optional<TypeId> resolveLiteral(const atomic::LiteralFeature &literal, const ast::Node &node) const;
    std::optional<TypeId> resolveLiteral(const atomic::LiteralInfo &literal, const ast::Node &node) const;
    std::optional<TypeId> resolveLiteral(const ast::Node &node) const;

    // Operation semantics ---------------------------------------------------
    registry::RegisterStatus registerOperation(
        const atomic::OperationFeature &operation,
        std::vector<TypeId> operands,
        TypeId result,
        ExtensionSet extensions = {}
    );
    registry::RegisterStatus registerOperation(
        const atomic::OperationInfo &operation,
        OperationSignature signature
    );

    std::optional<OperationResolution> resolveOperation(
        const atomic::OperationFeature &operation,
        std::span<const TypeId> operands
    ) const;
    std::optional<OperationResolution> resolveOperation(
        const atomic::OperationInfo &operation,
        std::span<const TypeId> operands
    ) const;

    OperationResolutionResult resolveOperationDetailed(
        const atomic::OperationFeature &operation,
        std::span<const TypeId> operands
    ) const;
    OperationResolutionResult resolveOperationDetailed(
        const atomic::OperationInfo &operation,
        std::span<const TypeId> operands
    ) const;

    void registerSemanticRule(
        const atomic::OperationFeature &operation,
        std::shared_ptr<const OperationSemanticRule> rule
    );
    void registerSemanticRule(
        const atomic::OperationInfo &operation,
        std::shared_ptr<const OperationSemanticRule> rule
    );

    template <typename Rule, typename... Args>
    Rule &emplaceSemanticRule(const atomic::OperationFeature &operation, Args &&...args) {
        static_assert(
            std::is_base_of_v<OperationSemanticRule, Rule>,
            "Rule must derive from OperationSemanticRule"
        );
        auto rule = std::make_shared<Rule>(std::forward<Args>(args)...);
        Rule &reference = *rule;
        registerSemanticRule(operation, std::move(rule));
        return reference;
    }

    registry::DuplicatePolicy duplicatePolicy() const noexcept;

private:
    struct LiteralBinding {
        std::string featureId{};
        std::string nodeKind{};
        LiteralTypeResolver resolver{};
    };

    TypeControllerOptions options_{};
    std::unordered_map<TypeId, std::shared_ptr<const TypeDefinition>, TypeIdHash> types_{};
    std::vector<TypeId> typeIds_{};
    std::unordered_set<TypeId, TypeIdHash> declarations_{};
    std::unordered_map<
        TypeId,
        std::unordered_map<TypeId, TypeConversion, TypeIdHash>,
        TypeIdHash
    > conversions_{};

    std::unordered_map<std::string, LiteralBinding> literalBindingsByFeature_{};
    std::unordered_map<std::string, std::vector<std::string>> literalFeaturesByNodeKind_{};
    std::unordered_map<std::string, std::vector<std::shared_ptr<const OperationSignature>>> operations_{};
    std::unordered_map<std::string, std::vector<std::shared_ptr<const OperationSemanticRule>>> semanticRules_{};

    static void validateTypeDefinition(const TypeDefinition &type);
    void validatePrimitive(const PrimitiveType &type) const;
    void validateConversion(const TypeId &source, const TypeConversion &conversion) const;
    registry::RegisterStatus commitPrimitive(
        PrimitiveType type,
        std::unordered_map<TypeId, TypeConversion, TypeIdHash> conversions
    );
    static void validateLiteralInfo(const atomic::LiteralInfo &literal);
    static void validateOperationInfo(const atomic::OperationInfo &operation);
    void validateSignature(const atomic::OperationInfo &operation, const OperationSignature &signature) const;
    OperationResolution materializeSemanticResolution(
        std::span<const TypeId> operands,
        SemanticOperationResolution resolution
    ) const;

    static std::size_t expectedArity(const atomic::OperationInfo &operation) noexcept;
    std::optional<TypeId> resolveLiteralBinding(const LiteralBinding &binding, const ast::Node &node) const;
    OperationResolutionResult resolveRegisteredOperations(
        const std::string &operationId,
        std::span<const TypeId> operands
    ) const;
    OperationResolutionResult resolveCustomRules(
        const std::string &operationId,
        std::span<const TypeId> operands
    ) const;
};

} // namespace novac::assets::types
