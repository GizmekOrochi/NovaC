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

/**
 * @brief Configuration options used by TypeController.
 */
struct TypeControllerOptions {
    /** @brief Policy applied when a registration conflicts with an existing entry. */
    registry::DuplicatePolicy duplicatePolicy{registry::DuplicatePolicy::Error};
};

/**
 * @brief Central registry and semantic resolver for language-defined types.
 *
 * Type descriptors are nominal by default. Aliases canonicalize names without
 * introducing conversions, while conversions remain explicit directed
 * relations owned by the controller.
 *
 * The controller is independent from EngineController and can be used only by
 * languages that need NovaC's type-system facilities.
 */
class TypeController final {
public:
    /**
     * @brief Fluent builder used to configure and register a PrimitiveType.
     *
     * The builder accumulates the primitive descriptor and optional pending
     * conversions. Nothing is registered until commit() is called.
     */
    class PrimitiveBuilder final {
    public:
        /**
         * @brief Creates a primitive builder owned by a TypeController.
         * @param controller Controller that receives the committed primitive.
         * @param id Identifier assigned to the primitive.
         */
        PrimitiveBuilder(TypeController &controller, TypeId id);

        /** @brief Sets the semantic/value width in bits. */
        PrimitiveBuilder &bits(std::size_t bitWidth);
        /** @brief Sets the physical storage width in bits. */
        PrimitiveBuilder &storageBits(std::size_t storageBits);
        /** @brief Sets storage alignment in bytes. */
        PrimitiveBuilder &alignment(std::size_t alignmentBytes);
        /** @brief Sets primitive signedness explicitly. */
        PrimitiveBuilder &signedness(PrimitiveSignedness value);
        /** @brief Marks the primitive as signed. */
        PrimitiveBuilder &signedType();
        /** @brief Marks the primitive as unsigned. */
        PrimitiveBuilder &unsignedType();
        /** @brief Marks signedness as not applicable. */
        PrimitiveBuilder &signless();

        /**
         * @brief Replaces the primitive representation descriptor.
         * @tparam Representation Type deriving from PrimitiveRepresentation.
         * @tparam Args Constructor argument types.
         * @param args Arguments forwarded to the representation constructor.
         * @return This builder.
         */
        template <typename Representation, typename... Args>
        PrimitiveBuilder &representation(Args &&...args) {
            static_assert(std::is_base_of_v<PrimitiveRepresentation, Representation>, "Representation must derive from PrimitiveRepresentation");
            type_.representation = std::make_shared<Representation>(std::forward<Args>(args)...);
            return *this;
        }

        /**
         * @brief Attaches strongly typed metadata to the primitive descriptor.
         * @tparam Extension Extension type.
         * @tparam Args Constructor argument types.
         * @param args Arguments forwarded to the extension constructor.
         * @return This builder.
         */
        template <typename Extension, typename... Args>
        PrimitiveBuilder &extension(Args &&...args) {
            type_.extensions.emplace<Extension>(std::forward<Args>(args)...);
            return *this;
        }

        /**
         * @brief Declares a conversion from the primitive being built.
         * @param target Conversion target type.
         * @param kind Conversion category.
         * @param rank Relative conversion cost used by overload resolution.
         * @return This builder.
         */
        PrimitiveBuilder &conversionTo(TypeId target, ConversionKind kind = ConversionKind::Explicit, std::size_t rank = 1);

        /**
         * @brief Attaches metadata to a conversion previously declared by conversionTo().
         * @tparam Extension Extension type.
         * @tparam Args Constructor argument types.
         * @param target Target identifying the pending conversion.
         * @param args Arguments forwarded to the extension constructor.
         * @return This builder.
         * @throws std::runtime_error if no pending conversion exists for target.
         */
        template <typename Extension, typename... Args>
        PrimitiveBuilder &conversionExtension(const TypeId &target, Args &&...args) {
            auto it{pendingConversions_.find(target)};
            if (it == pendingConversions_.end())
                throw std::runtime_error("TypeController::PrimitiveBuilder::conversionExtension: conversion must be declared first");

            it->second.extensions.emplace<Extension>(std::forward<Args>(args)...);
            return *this;
        }

        /**
         * @brief Returns the primitive descriptor currently accumulated by the builder.
         */
        const PrimitiveType &definition() const noexcept { return type_; }

        /**
         * @brief Validates and registers the primitive and its pending conversions.
         * @return Registration status according to the controller duplicate policy.
         */
        registry::RegisterStatus commit();

    private:
        TypeController *controller_{nullptr};
        PrimitiveType type_{};
        std::unordered_map<TypeId, TypeConversion, TypeIdHash> pendingConversions_{};
    };

    /**
     * @brief Constructs a type controller.
     * @param options Registration and duplicate-handling options.
     */
    explicit TypeController(TypeControllerOptions options = {});

    // Generic type registry

    /**
     * @brief Declares a type name that may be defined later.
     * @param id Type identifier to reserve.
     */
    void declareType(TypeId id);

    /**
     * @brief Declares a type name that may be defined later.
     * @param name Source-level type name.
     */
    void declareType(std::string name) { declareType(TypeId{std::move(name)}); }

    /**
     * @brief Checks whether a type identifier has been declared.
     * @param id Type identifier.
     * @return true when the name is known as a declaration.
     */
    bool isDeclared(const TypeId &id) const;

    /**
     * @brief Registers a generic custom type descriptor.
     * @param type Descriptor to transfer into the controller.
     * @return Registration status.
     */
    registry::RegisterStatus registerType(std::unique_ptr<TypeDefinition> type);

    /** @brief Checks whether a concrete type exists for id. */
    bool hasType(const TypeId &id) const;

    /** @brief Checks whether a concrete type exists for name. */
    bool hasType(const std::string &name) const { return hasType(TypeId{name}); }

    /**
     * @brief Finds a registered type.
     * @param id Type identifier.
     * @return Pointer to the descriptor, or nullptr when absent.
     */
    const TypeDefinition *findType(const TypeId &id) const;

    /**
     * @brief Retrieves a registered type.
     * @param id Type identifier.
     * @return Registered descriptor.
     */
    const TypeDefinition &requireType(const TypeId &id) const;

    /** @brief Returns registered type identifiers in registration order. */
    const std::vector<TypeId> &typeIds() const noexcept;

    /** @brief Returns the number of registered concrete types. */
    std::size_t typeCount() const noexcept;

    // Aliases and canonical identity

    /**
     * @brief Registers an alias for another type identifier.
     * @param alias Alias name.
     * @param target Target type or alias.
     * @return Registration status.
     */
    registry::RegisterStatus registerAlias(TypeId alias, TypeId target);

    /**
     * @brief Registers an alias from a source-level name.
     * @param alias Alias name.
     * @param target Target type or alias.
     * @return Registration status.
     */
    registry::RegisterStatus registerAlias(std::string alias, TypeId target) {
        return registerAlias(TypeId{std::move(alias)}, std::move(target));
    }

    /** @brief Checks whether id is registered as an alias. */
    bool hasAlias(const TypeId &id) const noexcept;

    /**
     * @brief Returns the immediate target of an alias.
     * @param id Alias identifier.
     * @return Direct alias target, or std::nullopt when id is not an alias.
     */
    std::optional<TypeId> aliasTarget(const TypeId &id) const;

    /**
     * @brief Resolves an alias chain to its canonical type identifier.
     * @param id Type or alias identifier.
     * @return Canonical identifier.
     */
    TypeId canonical(TypeId id) const;

    /**
     * @brief Tests nominal equivalence after alias canonicalization.
     * @param left First type identifier.
     * @param right Second type identifier.
     * @return true when both identifiers canonicalize to the same type.
     */
    bool equivalent(const TypeId &left, const TypeId &right) const;

    // Primitive helpers

    /** @brief Starts fluent construction of a primitive type. */
    PrimitiveBuilder definePrimitive(TypeId id);

    /** @brief Starts fluent construction of a primitive type from a name. */
    PrimitiveBuilder definePrimitive(std::string name) { return definePrimitive(TypeId{std::move(name)}); }

    /**
     * @brief Registers an already constructed primitive descriptor.
     * @param type Primitive descriptor to register.
     * @return Registration status.
     */
    registry::RegisterStatus registerPrimitive(PrimitiveType type);

    /** @brief Checks whether id resolves to a registered PrimitiveType. */
    bool hasPrimitive(const TypeId &id) const;

    /** @brief Checks whether name resolves to a registered PrimitiveType. */
    bool hasPrimitive(const std::string &name) const { return hasPrimitive(TypeId{name}); }

    /**
     * @brief Finds a registered primitive descriptor.
     * @param id Type or alias identifier.
     * @return Primitive descriptor, or nullptr when the resolved type is absent or non-primitive.
     */
    const PrimitiveType *findPrimitive(const TypeId &id) const;

    /**
     * @brief Retrieves a registered primitive descriptor.
     * @param id Type or alias identifier.
     * @return Primitive descriptor.
     */
    const PrimitiveType &requirePrimitive(const TypeId &id) const;

    /** @brief Retrieves a registered primitive descriptor by name. */
    const PrimitiveType &requirePrimitive(const std::string &name) const { return requirePrimitive(TypeId{name}); }

    /** @brief Returns the identifiers of all registered primitive descriptors. */
    std::vector<TypeId> primitiveIds() const;

    /** @brief Returns the number of registered primitive descriptors. */
    std::size_t primitiveCount() const noexcept;

    // Type relations --------------------------------------------------------

    /**
     * @brief Registers a directed conversion between two types.
     * @param source Source type.
     * @param target Target type.
     * @param kind Conversion category.
     * @param rank Relative overload-resolution cost.
     * @param extensions Optional typed metadata associated with the conversion.
     * @return Registration status.
     */
    registry::RegisterStatus registerConversion(TypeId source, TypeId target, ConversionKind kind = ConversionKind::Explicit, std::size_t rank = 1, ExtensionSet extensions = {});

    /**
     * @brief Finds a registered direct conversion.
     * @param source Source type.
     * @param target Target type.
     * @return Conversion descriptor, or nullptr when no direct relation exists.
     */
    const TypeConversion *findConversion(const TypeId &source, const TypeId &target) const;

    /**
     * @brief Checks whether a direct implicit conversion exists.
     * @param source Source type.
     * @param target Target type.
     * @return true when the types are equivalent or a direct implicit conversion is registered.
     */
    bool canImplicitlyConvert(const TypeId &source, const TypeId &target) const;

    /**
     * @brief Validates unresolved declarations, aliases and cross-type references.
     */
    void validate() const;

    /**
     * @brief Validates and prevents further mutation of this type configuration.
     */
    void finalize();

    /** @brief Reports whether finalize() has been called successfully. */
    bool finalized() const noexcept { return finalized_; }

    // Literal semantics

    /**
     * @brief Binds a LiteralFeature to a fixed result type.
     * @param literal Literal feature.
     * @param type Type produced by the literal.
     * @return Registration status.
     */
    registry::RegisterStatus bindLiteral(const atomic::LiteralFeature &literal, TypeId type);

    /** @brief Binds LiteralInfo to a fixed result type. */
    registry::RegisterStatus bindLiteral(const atomic::LiteralInfo &literal, TypeId type);

    /**
     * @brief Binds a LiteralFeature to a dynamic type resolver.
     * @param literal Literal feature.
     * @param resolver Resolver invoked for matching AST nodes.
     * @return Registration status.
     */
    registry::RegisterStatus bindLiteral(const atomic::LiteralFeature &literal, LiteralTypeResolver resolver);

    /** @brief Binds LiteralInfo to a dynamic type resolver. */
    registry::RegisterStatus bindLiteral(const atomic::LiteralInfo &literal, LiteralTypeResolver resolver);

    /**
     * @brief Resolves the type of a literal through a specific LiteralFeature.
     * @param literal Literal feature selecting the binding.
     * @param node Literal AST node.
     * @return Resolved canonical type, or std::nullopt when unresolved.
     */
    std::optional<TypeId> resolveLiteral(const atomic::LiteralFeature &literal, const ast::Node &node) const;

    /** @brief Resolves the type of a literal through LiteralInfo. */
    std::optional<TypeId> resolveLiteral(const atomic::LiteralInfo &literal, const ast::Node &node) const;

    /**
     * @brief Resolves a literal using bindings associated with the node kind.
     * @param node Literal AST node.
     * @return Resolved canonical type, or std::nullopt when unresolved.
     */
    std::optional<TypeId> resolveLiteral(const ast::Node &node) const;

    // Operation semantics

    /**
     * @brief Registers a concrete overload for an OperationFeature.
     * @param operation Operation feature.
     * @param operands Ordered operand types.
     * @param result Result type.
     * @param extensions Optional metadata associated with the signature.
     * @return Registration status.
     */
    registry::RegisterStatus registerOperation(const atomic::OperationFeature &operation, std::vector<TypeId> operands, TypeId result, ExtensionSet extensions = {});

    /**
     * @brief Registers a concrete overload using OperationInfo.
     * @param operation Operation descriptor.
     * @param signature Signature to register.
     * @return Registration status.
     */
    registry::RegisterStatus registerOperation(const atomic::OperationInfo &operation, OperationSignature signature);

    /**
     * @brief Resolves an operation and returns only successful resolutions.
     * @param operation Operation feature.
     * @param operands Operand types.
     * @return Resolution when successful, otherwise std::nullopt.
     */
    std::optional<OperationResolution> resolveOperation(const atomic::OperationFeature &operation, std::span<const TypeId> operands) const;

    /** @brief Resolves an operation described by OperationInfo. */
    std::optional<OperationResolution> resolveOperation(const atomic::OperationInfo &operation, std::span<const TypeId> operands) const;

    /**
     * @brief Resolves an operation with explicit status and diagnostics.
     * @param operation Operation feature.
     * @param operands Operand types.
     * @return Detailed resolution result.
     */
    OperationResolutionResult resolveOperationDetailed(const atomic::OperationFeature &operation, std::span<const TypeId> operands) const;

    /** @brief Resolves an OperationInfo with explicit status and diagnostics. */
    OperationResolutionResult resolveOperationDetailed(const atomic::OperationInfo &operation, std::span<const TypeId> operands) const;

    /**
     * @brief Registers an owned fallback semantic rule for an operation.
     * @param operation Operation feature.
     * @param rule Rule transferred to the controller.
     */
    void registerSemanticRule(const atomic::OperationFeature &operation, std::unique_ptr<OperationSemanticRule> rule);

    /** @brief Registers an owned fallback semantic rule using OperationInfo. */
    void registerSemanticRule(const atomic::OperationInfo &operation, std::unique_ptr<OperationSemanticRule> rule);

    /**
     * @brief Constructs and registers a semantic rule.
     * @tparam Rule Type deriving from OperationSemanticRule.
     * @tparam Args Constructor argument types.
     * @param operation Operation feature receiving the rule.
     * @param args Arguments forwarded to Rule's constructor.
     * @return Const reference to the registered rule.
     */
    template <typename Rule, typename... Args>
    const Rule &emplaceSemanticRule(const atomic::OperationFeature &operation, Args &&...args) {
        static_assert(std::is_base_of_v<OperationSemanticRule, Rule>, "Rule must derive from OperationSemanticRule");
        auto rule{std::make_unique<Rule>(std::forward<Args>(args)...)};
        const Rule &reference{*rule};
        registerSemanticRule(operation, std::move(rule));
        return reference;
    }

    /**
     * @brief Returns the duplicate-registration policy used by this controller.
     */
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
    std::unordered_map<TypeId, TypeId, TypeIdHash> aliases_{};
    std::unordered_map<TypeId, std::unordered_map<TypeId, TypeConversion, TypeIdHash>, TypeIdHash> conversions_{};

    std::unordered_map<std::string, LiteralBinding> literalBindingsByFeature_{};
    std::unordered_map<std::string, std::vector<std::string>> literalFeaturesByNodeKind_{};
    std::unordered_map<std::string, std::vector<std::shared_ptr<const OperationSignature>>> operations_{};
    std::unordered_map<std::string, std::vector<std::shared_ptr<const OperationSemanticRule>>> semanticRules_{};
    bool finalized_{false};

    void ensureMutable(const char *owner) const;
    bool hasConcreteType(const TypeId &id) const noexcept;
    bool wouldCreateAliasCycle(const TypeId &alias, const TypeId &target) const;
    static void validateTypeDefinition(const TypeDefinition &type);
    void validatePrimitive(const PrimitiveType &type) const;
    void validateConversion(const TypeId &source, const TypeConversion &conversion) const;

    registry::RegisterStatus commitPrimitive(PrimitiveType type, std::unordered_map<TypeId, TypeConversion, TypeIdHash> conversions);

    static void validateLiteralInfo(const atomic::LiteralInfo &literal);
    static void validateOperationInfo(const atomic::OperationInfo &operation);
    void validateSignature(const atomic::OperationInfo &operation, const OperationSignature &signature) const;

    OperationResolution materializeSemanticResolution(std::span<const TypeId> operands, SemanticOperationResolution resolution) const;

    static std::size_t expectedArity(const atomic::OperationInfo &operation) noexcept;
    std::optional<TypeId> resolveLiteralBinding(const LiteralBinding &binding, const ast::Node &node) const;
    OperationResolutionResult resolveRegisteredOperations(const std::string &operationId, std::span<const TypeId> operands) const;
    OperationResolutionResult resolveCustomRules(const std::string &operationId, std::span<const TypeId> operands) const;
};

} // namespace novac::assets::types