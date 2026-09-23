#pragma once

#include "novac/assets/types/model/Type.hpp"
#include "novac/engine/syntax/Node.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace novac::assets::types {

class TypeController;

/**
 * @brief Resolves the type produced by a literal AST node.
 *
 * A resolver returns the type selected for the literal, or std::nullopt when
 * the literal cannot be typed by the associated binding.
 */
using LiteralTypeResolver = std::function<std::optional<TypeId>(const ast::Node &)>;

/**
 * @brief Describes one registered operation overload.
 *
 * The signature contains the ordered operand types, the resulting type, and
 * optional typed metadata associated with the overload.
 */
struct OperationSignature {
    /** @brief Ordered operand types accepted by the overload. */
    std::vector<TypeId> operands{};
    /** @brief Type produced by the operation. */
    TypeId result{};
    /** @brief Language-defined metadata attached to the overload. */
    ExtensionSet extensions{};
};

/**
 * @brief Materialized implicit conversion applied to one operation operand.
 */
struct ConversionStep {
    /** @brief Zero-based index of the converted operand. */
    std::size_t operandIndex{0};
    /** @brief Canonical source type of the operand. */
    TypeId source{};
    /** @brief Canonical target type required by the selected overload. */
    TypeId target{};
    /** @brief Registered conversion descriptor used for the step. */
    TypeConversion conversion{};
};

/**
 * @brief Successful result of operation type resolution.
 *
 * A resolution describes the resulting type, any conversions that must be
 * applied to operands, the selected registered signature when applicable, and
 * optional metadata supplied by the signature or a semantic rule.
 */
struct OperationResolution {
    /** @brief Type produced after applying the selected semantic resolution. */
    TypeId result{};
    /** @brief Ordered set of operand conversions required by the resolution. */
    std::vector<ConversionStep> conversions{};
    /** @brief Selected registered signature, or nullptr for a custom rule. */
    std::shared_ptr<const OperationSignature> signature{};
    /** @brief Metadata associated with the resolved operation. */
    ExtensionSet extensions{};

    /**
     * @brief Reports whether the resolution requires no operand conversion.
     * @return true when every operand already has the required type.
     */
    bool exact() const noexcept { return conversions.empty(); }
};

/**
 * @brief Declarative conversion request returned by a custom semantic rule.
 *
 * The rule only states which operand should become which type. TypeController
 * validates the request and materializes the registered conversion.
 */
struct SemanticConversionRequest {
    /** @brief Zero-based index of the operand to convert. */
    std::size_t operandIndex{0};
    /** @brief Requested canonical target type. */
    TypeId target{};
};

/**
 * @brief Declarative operation result produced by a custom semantic rule.
 *
 * Custom rules describe intent only. TypeController validates all referenced
 * types and conversions before producing an OperationResolution.
 */
struct SemanticOperationResolution {
    /** @brief Type produced by the semantic rule. */
    TypeId result{};
    /** @brief Operand conversion requests required by the rule. */
    std::vector<SemanticConversionRequest> conversions{};
    /** @brief Optional language-defined metadata attached to the result. */
    ExtensionSet extensions{};
};

/**
 * @brief Candidate retained for overload-resolution diagnostics.
 */
struct OperationCandidateDiagnostic {
    /** @brief Operand types expected by the candidate. */
    std::vector<TypeId> operands{};
    /** @brief Result type produced by the candidate. */
    TypeId result{};
    /** @brief Total implicit-conversion cost of the candidate. */
    std::size_t conversionCost{0};
};

/**
 * @brief Human-readable diagnostic information for operation resolution.
 */
struct OperationResolutionDiagnostic {
    /** @brief Diagnostic summary. */
    std::string message{};
    /** @brief Relevant overload candidates, when available. */
    std::vector<OperationCandidateDiagnostic> candidates{};
};

/**
 * @brief Status returned by detailed operation resolution.
 */
enum class OperationResolutionStatus {
    /** A valid operation resolution was found. */
    Resolved,
    /** No registered overload or semantic rule matched. */
    NoMatch,
    /** Multiple equally valid candidates prevented a unique resolution. */
    Ambiguous,
    /** At least one operand refers to an unknown type. */
    UnknownOperand,
    /** The supplied operand count does not match the operation arity. */
    InvalidArity,
    /** A registered semantic element produced an invalid configuration. */
    InvalidConfiguration
};

/**
 * @brief Detailed result of operation type resolution.
 *
 * Use ok() for the common success check. On failure, diagnostic contains the
 * explanation and may contain competing overload candidates.
 */
struct OperationResolutionResult {
    /** @brief Resolution status. */
    OperationResolutionStatus status{OperationResolutionStatus::NoMatch};
    /** @brief Successful resolution when status is Resolved. */
    std::optional<OperationResolution> resolution{};
    /** @brief Diagnostic information for success or failure analysis. */
    OperationResolutionDiagnostic diagnostic{};

    /**
     * @brief Builds a successful detailed result.
     * @param value Materialized operation resolution.
     * @return A result with status Resolved.
     */
    static OperationResolutionResult resolved(OperationResolution value) {
        return {OperationResolutionStatus::Resolved, std::move(value), {}};
    }

    /**
     * @brief Builds a result indicating that no overload matched.
     * @param message Optional diagnostic message.
     * @return A result with status NoMatch.
     */
    static OperationResolutionResult noMatch(std::string message = {}) {
        return {OperationResolutionStatus::NoMatch, std::nullopt, {std::move(message), {}}};
    }

    /**
     * @brief Builds an ambiguous-overload result.
     * @param message Optional diagnostic message.
     * @param candidates Equally viable overload candidates.
     * @return A result with status Ambiguous.
     */
    static OperationResolutionResult ambiguous(std::string message = {}, std::vector<OperationCandidateDiagnostic> candidates = {}) {
        return {OperationResolutionStatus::Ambiguous, std::nullopt, {std::move(message), std::move(candidates)}};
    }

    /**
     * @brief Builds a result for an unknown operand type.
     * @param message Optional diagnostic message.
     * @return A result with status UnknownOperand.
     */
    static OperationResolutionResult unknownOperand(std::string message = {}) {
        return {OperationResolutionStatus::UnknownOperand, std::nullopt, {std::move(message), {}}};
    }

    /**
     * @brief Builds a result for an invalid operand count.
     * @param message Optional diagnostic message.
     * @return A result with status InvalidArity.
     */
    static OperationResolutionResult invalidArity(std::string message = {}) {
        return {OperationResolutionStatus::InvalidArity, std::nullopt, {std::move(message), {}}};
    }

    /**
     * @brief Builds a result for an invalid registered semantic configuration.
     * @param message Optional diagnostic message.
     * @return A result with status InvalidConfiguration.
     */
    static OperationResolutionResult invalidConfiguration(std::string message = {}) {
        return {OperationResolutionStatus::InvalidConfiguration, std::nullopt, {std::move(message), {}}};
    }

    /**
     * @brief Reports whether resolution succeeded.
     * @return true only when status is Resolved and a resolution is present.
     */
    bool ok() const noexcept {
        return status == OperationResolutionStatus::Resolved && resolution.has_value();
    }
};

/**
 * @brief Fallback semantic rule for operation typing.
 *
 * Concrete registered overloads are resolved before custom rules. A rule
 * returns std::nullopt when it does not apply to the supplied operand types.
 */
class OperationSemanticRule {
public:
    /** @brief Virtual destructor for custom semantic rules. */
    virtual ~OperationSemanticRule() = default;

    /**
     * @brief Returns the priority used when several semantic rules match.
     * @return Rule priority. Higher-level selection semantics are controlled by TypeController.
     */
    virtual int priority() const noexcept { return 0; }

    /**
     * @brief Attempts to resolve an operation for the supplied operand types.
     * @param types Type system used to inspect types and conversions.
     * @param operands Canonical operand types supplied to the operation.
     * @return A declarative resolution when the rule matches, otherwise std::nullopt.
     */
    virtual std::optional<SemanticOperationResolution> resolve(const TypeController &types, std::span<const TypeId> operands) const = 0;
};

} // namespace novac::assets::types