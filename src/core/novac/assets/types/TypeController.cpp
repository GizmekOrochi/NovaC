#include "novac/assets/types/TypeController.hpp"

#include "novac/assets/atomic/LiteralFeature.hpp"
#include "novac/assets/atomic/OperationFeature.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace novac::assets::types {
namespace {

void requireTypeId(const TypeId &id, const char *message) {
    if (!id.valid()) {
        throw std::runtime_error(message);
    }
}

bool sameOperands(const OperationSignature &left, const OperationSignature &right) {
    return left.operands == right.operands;
}

} // namespace

// PrimitiveBuilder ----------------------------------------------------------

TypeController::PrimitiveBuilder::PrimitiveBuilder(TypeController &controller, TypeId id)
    : controller_{&controller}, type_{std::move(id)} {}

TypeController::PrimitiveBuilder &TypeController::PrimitiveBuilder::bits(std::size_t bitWidth) {
    type_.bitWidth = bitWidth;
    return *this;
}

TypeController::PrimitiveBuilder &TypeController::PrimitiveBuilder::storageBits(std::size_t storageBits) {
    type_.storageBits = storageBits;
    return *this;
}

TypeController::PrimitiveBuilder &TypeController::PrimitiveBuilder::alignment(std::size_t alignmentBytes) {
    type_.alignment = alignmentBytes;
    return *this;
}

TypeController::PrimitiveBuilder &TypeController::PrimitiveBuilder::signedness(PrimitiveSignedness value) {
    type_.signedness = value;
    return *this;
}

TypeController::PrimitiveBuilder &TypeController::PrimitiveBuilder::signedType() {
    return signedness(PrimitiveSignedness::Signed);
}

TypeController::PrimitiveBuilder &TypeController::PrimitiveBuilder::unsignedType() {
    return signedness(PrimitiveSignedness::Unsigned);
}

TypeController::PrimitiveBuilder &TypeController::PrimitiveBuilder::signless() {
    return signedness(PrimitiveSignedness::NotApplicable);
}

TypeController::PrimitiveBuilder &TypeController::PrimitiveBuilder::conversionTo(
    TypeId target,
    ConversionKind kind,
    std::size_t rank
) {
    requireTypeId(target, "TypeController::PrimitiveBuilder::conversionTo: target type cannot be empty");
    if (kind == ConversionKind::Implicit && rank == 0) {
        throw std::runtime_error(
            "TypeController::PrimitiveBuilder::conversionTo: implicit conversion rank must be greater than zero"
        );
    }

    const TypeId key = target;
    pendingConversions_.insert_or_assign(key, TypeConversion{std::move(target), kind, rank, {}});
    return *this;
}

registry::RegisterStatus TypeController::PrimitiveBuilder::commit() {
    if (controller_ == nullptr) {
        throw std::runtime_error("TypeController::PrimitiveBuilder::commit: builder has no controller");
    }
    return controller_->commitPrimitive(std::move(type_), std::move(pendingConversions_));
}

// TypeController ------------------------------------------------------------

TypeController::TypeController(TypeControllerOptions options) : options_{options} {}

void TypeController::declareType(TypeId id) {
    requireTypeId(id, "TypeController::declareType: type id cannot be empty");
    if (!hasType(id)) {
        declarations_.insert(std::move(id));
    }
}

bool TypeController::isDeclared(const TypeId &id) const noexcept {
    return hasType(id) || declarations_.find(id) != declarations_.end();
}

registry::RegisterStatus TypeController::registerType(std::unique_ptr<TypeDefinition> type) {
    if (!type) {
        throw std::runtime_error("TypeController::registerType: type cannot be null");
    }
    validateTypeDefinition(*type);

    const TypeId id = type->id;
    auto found = types_.find(id);

    type->freeze();
    std::shared_ptr<const TypeDefinition> stored{std::move(type)};

    if (found == types_.end()) {
        types_.emplace(id, std::move(stored));
        typeIds_.push_back(id);
        declarations_.erase(id);
        return registry::RegisterStatus::Inserted;
    }

    switch (options_.duplicatePolicy) {
        case registry::DuplicatePolicy::Error:
            throw std::runtime_error("TypeController::registerType: duplicate type '" + id.name + "'");
        case registry::DuplicatePolicy::Ignore:
            return registry::RegisterStatus::Ignored;
        case registry::DuplicatePolicy::Replace:
            found->second = std::move(stored);
            declarations_.erase(id);
            return registry::RegisterStatus::Replaced;
    }

    throw std::runtime_error("TypeController::registerType: unknown duplicate policy");
}

bool TypeController::hasType(const TypeId &id) const noexcept {
    return types_.find(id) != types_.end();
}

const TypeDefinition *TypeController::findType(const TypeId &id) const noexcept {
    const auto it = types_.find(id);
    return it == types_.end() ? nullptr : it->second.get();
}

const TypeDefinition &TypeController::requireType(const TypeId &id) const {
    const TypeDefinition *type = findType(id);
    if (!type) {
        throw std::runtime_error("TypeController::requireType: unknown type '" + id.name + "'");
    }
    return *type;
}

const std::vector<TypeId> &TypeController::typeIds() const noexcept {
    return typeIds_;
}

std::size_t TypeController::typeCount() const noexcept {
    return types_.size();
}

TypeController::PrimitiveBuilder TypeController::definePrimitive(TypeId id) {
    return PrimitiveBuilder{*this, std::move(id)};
}

registry::RegisterStatus TypeController::registerPrimitive(PrimitiveType type) {
    return commitPrimitive(std::move(type), {});
}

registry::RegisterStatus TypeController::commitPrimitive(
    PrimitiveType type,
    std::unordered_map<TypeId, TypeConversion, TypeIdHash> conversions
) {
    if (type.storageBits == 0) {
        type.storageBits = type.bitWidth;
    }
    validatePrimitive(type);

    for (const auto &[target, conversion] : conversions) {
        if (!(target == conversion.target)) {
            throw std::runtime_error("TypeController::PrimitiveBuilder: conversion key does not match target");
        }
        requireTypeId(target, "TypeController::PrimitiveBuilder: conversion target cannot be empty");
        if (!(target == type.id) && !isDeclared(target)) {
            throw std::runtime_error(
                "TypeController::PrimitiveBuilder: conversion target '" + target.name +
                "' is neither registered nor declared"
            );
        }
        if (conversion.kind == ConversionKind::Implicit && conversion.rank == 0) {
            throw std::runtime_error(
                "TypeController::PrimitiveBuilder: implicit conversion rank must be greater than zero"
            );
        }
        if (findConversion(type.id, target) != nullptr &&
            options_.duplicatePolicy == registry::DuplicatePolicy::Error) {
            throw std::runtime_error(
                "TypeController::PrimitiveBuilder: duplicate conversion from '" + type.id.name +
                "' to '" + target.name + "'"
            );
        }
    }

    const TypeId id = type.id;
    const registry::RegisterStatus status = registerType(std::make_unique<PrimitiveType>(std::move(type)));
    if (status == registry::RegisterStatus::Ignored) {
        return status;
    }

    if (status == registry::RegisterStatus::Replaced) {
        conversions_.erase(id);
    }
    for (auto &[target, conversion] : conversions) {
        if (options_.duplicatePolicy == registry::DuplicatePolicy::Ignore && findConversion(id, target) != nullptr) {
            continue;
        }
        conversion.extensions.freeze();
        conversions_[id].insert_or_assign(target, std::move(conversion));
    }
    return status;
}

bool TypeController::hasPrimitive(const TypeId &id) const noexcept {
    return findPrimitive(id) != nullptr;
}

const PrimitiveType *TypeController::findPrimitive(const TypeId &id) const noexcept {
    return dynamic_cast<const PrimitiveType *>(findType(id));
}

const PrimitiveType &TypeController::requirePrimitive(const TypeId &id) const {
    const PrimitiveType *type = findPrimitive(id);
    if (!type) {
        throw std::runtime_error("TypeController::requirePrimitive: unknown primitive type '" + id.name + "'");
    }
    return *type;
}

std::vector<TypeId> TypeController::primitiveIds() const {
    std::vector<TypeId> result;
    result.reserve(types_.size());
    for (const TypeId &id : typeIds_) {
        if (hasPrimitive(id)) {
            result.push_back(id);
        }
    }
    return result;
}

std::size_t TypeController::primitiveCount() const noexcept {
    std::size_t count = 0;
    for (const auto &[_, type] : types_) {
        if (dynamic_cast<const PrimitiveType *>(type.get()) != nullptr) {
            ++count;
        }
    }
    return count;
}

registry::RegisterStatus TypeController::registerConversion(
    TypeId source,
    TypeId target,
    ConversionKind kind,
    std::size_t rank,
    ExtensionSet extensions
) {
    requireTypeId(source, "TypeController::registerConversion: source type cannot be empty");
    requireTypeId(target, "TypeController::registerConversion: target type cannot be empty");
    TypeConversion conversion{target, kind, rank, std::move(extensions)};
    validateConversion(source, conversion);
    conversion.extensions.freeze();

    auto &bucket = conversions_[source];
    const auto found = bucket.find(target);
    if (found == bucket.end()) {
        bucket.emplace(std::move(target), std::move(conversion));
        return registry::RegisterStatus::Inserted;
    }

    switch (options_.duplicatePolicy) {
        case registry::DuplicatePolicy::Error:
            throw std::runtime_error(
                "TypeController::registerConversion: duplicate conversion from '" + source.name +
                "' to '" + target.name + "'"
            );
        case registry::DuplicatePolicy::Ignore:
            return registry::RegisterStatus::Ignored;
        case registry::DuplicatePolicy::Replace:
            found->second = std::move(conversion);
            return registry::RegisterStatus::Replaced;
    }
    throw std::runtime_error("TypeController::registerConversion: unknown duplicate policy");
}

const TypeConversion *TypeController::findConversion(const TypeId &source, const TypeId &target) const noexcept {
    const auto sourceIt = conversions_.find(source);
    if (sourceIt == conversions_.end()) {
        return nullptr;
    }
    const auto targetIt = sourceIt->second.find(target);
    return targetIt == sourceIt->second.end() ? nullptr : &targetIt->second;
}

bool TypeController::canImplicitlyConvert(const TypeId &source, const TypeId &target) const noexcept {
    if (source == target) {
        return true;
    }
    const TypeConversion *conversion = findConversion(source, target);
    return conversion && conversion->isImplicit();
}

void TypeController::validate() const {
    if (!declarations_.empty()) {
        const auto &id = *declarations_.begin();
        throw std::runtime_error(
            "TypeController::validate: unresolved declared type '" + id.name + "'"
        );
    }
    for (const auto &[source, targets] : conversions_) {
        if (!hasType(source)) {
            throw std::runtime_error(
                "TypeController::validate: conversion source '" + source.name + "' is not registered"
            );
        }
        for (const auto &[target, _] : targets) {
            if (!hasType(target)) {
                throw std::runtime_error(
                    "TypeController::validate: conversion target '" + target.name + "' is not registered"
                );
            }
        }
    }
}

// Literal semantics ---------------------------------------------------------

registry::RegisterStatus TypeController::bindLiteral(const atomic::LiteralFeature &literal, TypeId type) {
    return bindLiteral(literal.info(), std::move(type));
}

registry::RegisterStatus TypeController::bindLiteral(const atomic::LiteralInfo &literal, TypeId type) {
    if (!hasType(type)) {
        throw std::runtime_error("TypeController::bindLiteral: unknown literal type '" + type.name + "'");
    }
    return bindLiteral(literal, [type = std::move(type)](const ast::Node &) -> std::optional<TypeId> {
        return type;
    });
}

registry::RegisterStatus TypeController::bindLiteral(
    const atomic::LiteralFeature &literal,
    LiteralTypeResolver resolver
) {
    return bindLiteral(literal.info(), std::move(resolver));
}

registry::RegisterStatus TypeController::bindLiteral(
    const atomic::LiteralInfo &literal,
    LiteralTypeResolver resolver
) {
    validateLiteralInfo(literal);
    if (!resolver) {
        throw std::runtime_error("TypeController::bindLiteral: resolver cannot be empty");
    }

    LiteralBinding binding{literal.id, literal.nodeKind, std::move(resolver)};
    const auto found = literalBindingsByFeature_.find(literal.id);
    if (found == literalBindingsByFeature_.end()) {
        literalBindingsByFeature_.emplace(literal.id, std::move(binding));
        literalFeaturesByNodeKind_[literal.nodeKind].push_back(literal.id);
        return registry::RegisterStatus::Inserted;
    }

    switch (options_.duplicatePolicy) {
        case registry::DuplicatePolicy::Error:
            throw std::runtime_error(
                "TypeController::bindLiteral: literal feature '" + literal.id + "' already has typing semantics"
            );
        case registry::DuplicatePolicy::Ignore:
            return registry::RegisterStatus::Ignored;
        case registry::DuplicatePolicy::Replace: {
            const std::string oldKind = found->second.nodeKind;
            found->second = std::move(binding);
            if (oldKind != literal.nodeKind) {
                auto &oldList = literalFeaturesByNodeKind_[oldKind];
                oldList.erase(std::remove(oldList.begin(), oldList.end(), literal.id), oldList.end());
                literalFeaturesByNodeKind_[literal.nodeKind].push_back(literal.id);
            }
            return registry::RegisterStatus::Replaced;
        }
    }

    throw std::runtime_error("TypeController::bindLiteral: unknown duplicate policy");
}

std::optional<TypeId> TypeController::resolveLiteralBinding(
    const LiteralBinding &binding,
    const ast::Node &node
) const {
    std::optional<TypeId> result = binding.resolver(node);
    if (result && !hasType(*result)) {
        throw std::runtime_error(
            "TypeController::resolveLiteral: resolver for literal feature '" +
            binding.featureId + "' returned unknown type '" + result->name + "'"
        );
    }
    return result;
}

std::optional<TypeId> TypeController::resolveLiteral(
    const atomic::LiteralFeature &literal,
    const ast::Node &node
) const {
    return resolveLiteral(literal.info(), node);
}

std::optional<TypeId> TypeController::resolveLiteral(
    const atomic::LiteralInfo &literal,
    const ast::Node &node
) const {
    validateLiteralInfo(literal);
    const auto found = literalBindingsByFeature_.find(literal.id);
    if (found == literalBindingsByFeature_.end()) {
        return std::nullopt;
    }
    if (found->second.nodeKind != node.kind()) {
        return std::nullopt;
    }
    return resolveLiteralBinding(found->second, node);
}

std::optional<TypeId> TypeController::resolveLiteral(const ast::Node &node) const {
    const auto byKind = literalFeaturesByNodeKind_.find(node.kind());
    if (byKind == literalFeaturesByNodeKind_.end()) {
        return std::nullopt;
    }

    std::optional<TypeId> resolved;
    for (const std::string &featureId : byKind->second) {
        const auto binding = literalBindingsByFeature_.find(featureId);
        if (binding == literalBindingsByFeature_.end()) {
            continue;
        }
        std::optional<TypeId> candidate = resolveLiteralBinding(binding->second, node);
        if (!candidate) {
            continue;
        }
        if (!resolved) {
            resolved = std::move(candidate);
        } else if (!(*resolved == *candidate)) {
            throw std::runtime_error(
                "TypeController::resolveLiteral: AST node kind '" + node.kind() +
                "' matches multiple literal features with different types; resolve using the LiteralFeature identity"
            );
        }
    }
    return resolved;
}

// Operation semantics -------------------------------------------------------

registry::RegisterStatus TypeController::registerOperation(
    const atomic::OperationFeature &operation,
    std::vector<TypeId> operands,
    TypeId result,
    ExtensionSet extensions
) {
    return registerOperation(
        operation.info(),
        OperationSignature{std::move(operands), std::move(result), std::move(extensions)}
    );
}

registry::RegisterStatus TypeController::registerOperation(
    const atomic::OperationInfo &operation,
    OperationSignature signature
) {
    validateOperationInfo(operation);
    validateSignature(operation, signature);
    signature.extensions.freeze();

    auto &bucket = operations_[operation.id];
    const auto existing = std::find_if(
        bucket.begin(), bucket.end(),
        [&](const std::shared_ptr<const OperationSignature> &candidate) {
            return sameOperands(*candidate, signature);
        }
    );

    auto stored = std::make_shared<const OperationSignature>(std::move(signature));
    if (existing == bucket.end()) {
        bucket.push_back(std::move(stored));
        return registry::RegisterStatus::Inserted;
    }

    switch (options_.duplicatePolicy) {
        case registry::DuplicatePolicy::Error:
            throw std::runtime_error(
                "TypeController::registerOperation: duplicate typed overload for operation '" + operation.id + "'"
            );
        case registry::DuplicatePolicy::Ignore:
            return registry::RegisterStatus::Ignored;
        case registry::DuplicatePolicy::Replace:
            *existing = std::move(stored);
            return registry::RegisterStatus::Replaced;
    }

    throw std::runtime_error("TypeController::registerOperation: unknown duplicate policy");
}

std::optional<OperationResolution> TypeController::resolveOperation(
    const atomic::OperationFeature &operation,
    std::span<const TypeId> operands
) const {
    return resolveOperation(operation.info(), operands);
}

std::optional<OperationResolution> TypeController::resolveOperation(
    const atomic::OperationInfo &operation,
    std::span<const TypeId> operands
) const {
    OperationResolutionResult result = resolveOperationDetailed(operation, operands);
    return result.ok() ? result.resolution : std::nullopt;
}

OperationResolutionResult TypeController::resolveOperationDetailed(
    const atomic::OperationFeature &operation,
    std::span<const TypeId> operands
) const {
    return resolveOperationDetailed(operation.info(), operands);
}

OperationResolutionResult TypeController::resolveOperationDetailed(
    const atomic::OperationInfo &operation,
    std::span<const TypeId> operands
) const {
    validateOperationInfo(operation);
    if (operands.size() != expectedArity(operation)) {
        return OperationResolutionResult::invalidArity();
    }
    for (const TypeId &operand : operands) {
        if (!hasType(operand)) {
            return OperationResolutionResult::unknownOperand();
        }
    }

    OperationResolutionResult fixed = resolveRegisteredOperations(operation.id, operands);
    if (fixed.status == OperationResolutionStatus::Resolved ||
        fixed.status == OperationResolutionStatus::Ambiguous) {
        return fixed;
    }
    return resolveCustomRules(operation.id, operands);
}

void TypeController::registerSemanticRule(
    const atomic::OperationFeature &operation,
    std::shared_ptr<const OperationSemanticRule> rule
) {
    registerSemanticRule(operation.info(), std::move(rule));
}

void TypeController::registerSemanticRule(
    const atomic::OperationInfo &operation,
    std::shared_ptr<const OperationSemanticRule> rule
) {
    validateOperationInfo(operation);
    if (!rule) {
        throw std::runtime_error("TypeController::registerSemanticRule: rule cannot be null");
    }
    semanticRules_[operation.id].push_back(std::move(rule));
}

OperationResolutionResult TypeController::resolveRegisteredOperations(
    const std::string &operationId,
    std::span<const TypeId> operands
) const {
    const auto bucket = operations_.find(operationId);
    if (bucket == operations_.end()) {
        return OperationResolutionResult::noMatch();
    }

    std::shared_ptr<const OperationSignature> best{};
    std::vector<ConversionStep> bestConversions{};
    std::size_t bestCost = std::numeric_limits<std::size_t>::max();
    bool ambiguous = false;

    for (const auto &signature : bucket->second) {
        if (signature->operands.size() != operands.size()) {
            continue;
        }

        std::vector<ConversionStep> conversions{};
        std::size_t cost = 0;
        bool viable = true;

        for (std::size_t index = 0; index < operands.size(); ++index) {
            if (operands[index] == signature->operands[index]) {
                continue;
            }

            const TypeConversion *conversion = findConversion(operands[index], signature->operands[index]);
            if (!conversion || !conversion->isImplicit()) {
                viable = false;
                break;
            }

            if (conversion->rank > std::numeric_limits<std::size_t>::max() - cost) {
                viable = false;
                break;
            }
            cost += conversion->rank;
            conversions.push_back(ConversionStep{index, operands[index], signature->operands[index], *conversion});
        }

        if (!viable) {
            continue;
        }

        if (cost < bestCost) {
            best = signature;
            bestConversions = std::move(conversions);
            bestCost = cost;
            ambiguous = false;
        } else if (cost == bestCost) {
            ambiguous = true;
        }
    }

    if (ambiguous) {
        return OperationResolutionResult::ambiguous();
    }
    if (!best) {
        return OperationResolutionResult::noMatch();
    }

    OperationResolution result;
    result.result = best->result;
    result.conversions = std::move(bestConversions);
    result.signature = best;
    result.extensions = best->extensions;
    return OperationResolutionResult::resolved(std::move(result));
}

OperationResolutionResult TypeController::resolveCustomRules(
    const std::string &operationId,
    std::span<const TypeId> operands
) const {
    const auto bucket = semanticRules_.find(operationId);
    if (bucket == semanticRules_.end()) {
        return OperationResolutionResult::noMatch();
    }

    int bestPriority = std::numeric_limits<int>::min();
    std::optional<OperationResolution> best{};
    bool ambiguous = false;

    for (const auto &rule : bucket->second) {
        std::optional<SemanticOperationResolution> semantic = rule->resolve(*this, operands);
        if (!semantic) {
            continue;
        }
        OperationResolution candidate = materializeSemanticResolution(operands, std::move(*semantic));

        if (!best || rule->priority() > bestPriority) {
            bestPriority = rule->priority();
            best = std::move(candidate);
            ambiguous = false;
        } else if (rule->priority() == bestPriority) {
            ambiguous = true;
        }
    }

    if (ambiguous) {
        return OperationResolutionResult::ambiguous();
    }
    if (!best) {
        return OperationResolutionResult::noMatch();
    }
    return OperationResolutionResult::resolved(std::move(*best));
}

registry::DuplicatePolicy TypeController::duplicatePolicy() const noexcept {
    return options_.duplicatePolicy;
}

// Validation ---------------------------------------------------------------

void TypeController::validateTypeDefinition(const TypeDefinition &type) {
    requireTypeId(type.id, "TypeController::registerType: type id cannot be empty");
}

void TypeController::validatePrimitive(const PrimitiveType &type) const {
    validateTypeDefinition(type);
    if (type.bitWidth == 0) {
        throw std::runtime_error(
            "TypeController::registerPrimitive: primitive '" + type.id.name + "' must use at least one bit"
        );
    }
    if (type.storageBits < type.bitWidth) {
        throw std::runtime_error(
            "TypeController::registerPrimitive: storage width for '" + type.id.name +
            "' cannot be smaller than its semantic bit width"
        );
    }
    if (type.alignment == 0) {
        throw std::runtime_error(
            "TypeController::registerPrimitive: alignment for '" + type.id.name + "' must be non-zero"
        );
    }
    if (!type.representation) {
        throw std::runtime_error("TypeController::registerPrimitive: primitive representation cannot be null");
    }
}

void TypeController::validateConversion(const TypeId &source, const TypeConversion &conversion) const {
    requireTypeId(source, "TypeController::registerConversion: source type cannot be empty");
    requireTypeId(conversion.target, "TypeController::registerConversion: target type cannot be empty");
    if (!isDeclared(source)) {
        throw std::runtime_error(
            "TypeController::registerConversion: source type '" + source.name + "' is neither registered nor declared"
        );
    }
    if (!isDeclared(conversion.target)) {
        throw std::runtime_error(
            "TypeController::registerConversion: target type '" + conversion.target.name +
            "' is neither registered nor declared"
        );
    }
    if (conversion.kind == ConversionKind::Implicit && conversion.rank == 0) {
        throw std::runtime_error(
            "TypeController::registerConversion: implicit conversion rank must be greater than zero"
        );
    }
}

void TypeController::validateLiteralInfo(const atomic::LiteralInfo &literal) {
    if (literal.id.empty()) {
        throw std::runtime_error("TypeController::bindLiteral: literal id cannot be empty");
    }
    if (literal.nodeKind.empty()) {
        throw std::runtime_error("TypeController::bindLiteral: literal node kind cannot be empty");
    }
}

void TypeController::validateOperationInfo(const atomic::OperationInfo &operation) {
    if (operation.id.empty()) {
        throw std::runtime_error("TypeController: operation id cannot be empty");
    }
}

std::size_t TypeController::expectedArity(const atomic::OperationInfo &operation) noexcept {
    switch (operation.arity) {
        case atomic::OperationArity::Unary:
        case atomic::OperationArity::Postfix:
            return 1;
        case atomic::OperationArity::Binary:
            return 2;
    }
    return 0;
}

void TypeController::validateSignature(
    const atomic::OperationInfo &operation,
    const OperationSignature &signature
) const {
    const std::size_t arity = expectedArity(operation);
    if (signature.operands.size() != arity) {
        throw std::runtime_error(
            "TypeController::registerOperation: operation '" + operation.id + "' expects " +
            std::to_string(arity) + " operand type(s), got " + std::to_string(signature.operands.size())
        );
    }
    for (const TypeId &operand : signature.operands) {
        requireTypeId(operand, "TypeController::registerOperation: operand type cannot be empty");
        if (!hasType(operand)) {
            throw std::runtime_error(
                "TypeController::registerOperation: unknown operand type '" + operand.name + "'"
            );
        }
    }
    requireTypeId(signature.result, "TypeController::registerOperation: result type cannot be empty");
    if (!hasType(signature.result)) {
        throw std::runtime_error(
            "TypeController::registerOperation: unknown result type '" + signature.result.name + "'"
        );
    }
}

OperationResolution TypeController::materializeSemanticResolution(
    std::span<const TypeId> operands,
    SemanticOperationResolution semantic
) const {
    requireTypeId(
        semantic.result,
        "TypeController::resolveOperation: semantic rule returned an empty result type"
    );
    if (!hasType(semantic.result)) {
        throw std::runtime_error(
            "TypeController::resolveOperation: semantic rule returned unknown result type '" +
            semantic.result.name + "'"
        );
    }

    OperationResolution result;
    result.result = std::move(semantic.result);
    result.extensions = std::move(semantic.extensions);
    result.extensions.freeze();

    std::vector<bool> converted(operands.size(), false);
    for (const SemanticConversionRequest &request : semantic.conversions) {
        if (request.operandIndex >= operands.size()) {
            throw std::runtime_error(
                "TypeController::resolveOperation: semantic rule returned an out-of-range operand index"
            );
        }
        if (converted[request.operandIndex]) {
            throw std::runtime_error(
                "TypeController::resolveOperation: semantic rule requested multiple conversions for one operand"
            );
        }
        converted[request.operandIndex] = true;
        requireTypeId(request.target, "TypeController::resolveOperation: conversion target cannot be empty");

        const TypeId &source = operands[request.operandIndex];
        if (source == request.target) {
            continue;
        }
        const TypeConversion *registered = findConversion(source, request.target);
        if (!registered || !registered->isImplicit()) {
            throw std::runtime_error(
                "TypeController::resolveOperation: semantic rule requested a conversion that is not a registered direct implicit conversion"
            );
        }
        result.conversions.push_back(
            ConversionStep{request.operandIndex, source, request.target, *registered}
        );
    }
    return result;
}

} // namespace novac::assets::types
