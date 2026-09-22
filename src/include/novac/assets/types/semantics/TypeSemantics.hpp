#pragma once

#include "novac/assets/types/model/Type.hpp"
#include "novac/engine/syntax/Node.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace novac::assets::types {

class TypeController;

using LiteralTypeResolver = std::function<std::optional<TypeId>(const ast::Node &)>;

struct OperationSignature {
    std::vector<TypeId> operands{};
    TypeId result{};
    ExtensionSet extensions{};
};

struct ConversionStep {
    std::size_t operandIndex{0};
    TypeId source{};
    TypeId target{};
    TypeConversion conversion{};
};

struct OperationResolution {
    TypeId result{};
    std::vector<ConversionStep> conversions{};
    std::shared_ptr<const OperationSignature> signature{};
    ExtensionSet extensions{};

    bool exact() const noexcept { return conversions.empty(); }
};

/** Declarative conversion request returned by custom semantic rules. */
struct SemanticConversionRequest {
    std::size_t operandIndex{0};
    TypeId target{};
};

/** Custom rules describe intent; TypeController materializes registered conversions. */
struct SemanticOperationResolution {
    TypeId result{};
    std::vector<SemanticConversionRequest> conversions{};
    ExtensionSet extensions{};
};

enum class OperationResolutionStatus {
    Resolved,
    NoMatch,
    Ambiguous,
    UnknownOperand,
    InvalidArity
};

struct OperationResolutionResult {
    OperationResolutionStatus status{OperationResolutionStatus::NoMatch};
    std::optional<OperationResolution> resolution{};

    static OperationResolutionResult resolved(OperationResolution value) {
        return {OperationResolutionStatus::Resolved, std::move(value)};
    }
    static OperationResolutionResult noMatch() noexcept { return {OperationResolutionStatus::NoMatch, std::nullopt}; }
    static OperationResolutionResult ambiguous() noexcept { return {OperationResolutionStatus::Ambiguous, std::nullopt}; }
    static OperationResolutionResult unknownOperand() noexcept { return {OperationResolutionStatus::UnknownOperand, std::nullopt}; }
    static OperationResolutionResult invalidArity() noexcept { return {OperationResolutionStatus::InvalidArity, std::nullopt}; }

    bool ok() const noexcept {
        return status == OperationResolutionStatus::Resolved && resolution.has_value();
    }
};

/** Fallback semantic rule. Concrete registered overloads are resolved first. */
class OperationSemanticRule {
public:
    virtual ~OperationSemanticRule() = default;
    virtual int priority() const noexcept { return 0; }
    virtual std::optional<SemanticOperationResolution> resolve(
        const TypeController &types,
        std::span<const TypeId> operands
    ) const = 0;
};

} // namespace novac::assets::types
