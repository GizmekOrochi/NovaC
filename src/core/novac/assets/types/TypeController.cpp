#include "novac/assets/types/TypeController.hpp"

#include "novac/assets/atomic/LiteralFeature.hpp"
#include "novac/assets/atomic/OperationFeature.hpp"

#include <algorithm>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace novac::assets::types {
namespace {

void requireTypeId(const TypeId &id, const char *message) {
    if (!id.valid())
        throw std::runtime_error(message);
}

bool sameOperands(const OperationSignature &left, const OperationSignature &right) {
    return left.operands == right.operands;
}

std::string formatTypeList(std::span<const TypeId> types) {
    std::ostringstream out;
    out << '(';
    for (size_t index{}; index < types.size(); ++index) {
        if (index != 0)
            out << ", ";
        out << types[index].name;
    }
    out << ')';
    return out.str();
}

} // namespace

// PrimitiveBuilder

TypeController::PrimitiveBuilder::PrimitiveBuilder(TypeController &controller, TypeId id)
    : controller_{&controller}, type_{std::move(id)} {}

TypeController::PrimitiveBuilder &TypeController::PrimitiveBuilder::bits(size_t bitWidth) {
    type_.bitWidth = bitWidth;
    return *this;
}

TypeController::PrimitiveBuilder &TypeController::PrimitiveBuilder::storageBits(size_t storageBits) {
    type_.storageBits = storageBits;
    return *this;
}

TypeController::PrimitiveBuilder &TypeController::PrimitiveBuilder::alignment(size_t alignmentBytes) {
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

TypeController::PrimitiveBuilder &TypeController::PrimitiveBuilder::conversionTo(TypeId target, ConversionKind kind, size_t rank) {
    requireTypeId(target, "TypeController::PrimitiveBuilder::conversionTo: target type cannot be empty");
    if (kind == ConversionKind::Implicit && rank == 0) {
        throw std::runtime_error("TypeController::PrimitiveBuilder::conversionTo: implicit conversion rank must be greater than zero");
    }

    const TypeId key{target};
    pendingConversions_.insert_or_assign(key, TypeConversion{std::move(target), kind, rank, {}});
    return *this;
}

registry::RegisterStatus TypeController::PrimitiveBuilder::commit() {
    if (controller_ == nullptr)
        throw std::runtime_error("TypeController::PrimitiveBuilder::commit: builder has no controller");
    return controller_->commitPrimitive(std::move(type_), std::move(pendingConversions_));
}

// TypeController

TypeController::TypeController(TypeControllerOptions options) : options_{options} {}

void TypeController::ensureMutable(const char *owner) const {
    if (finalized_)
        throw std::runtime_error(std::string{owner} + ": type configuration is finalized");
}

bool TypeController::hasConcreteType(const TypeId &id) const noexcept {
    return types_.find(id) != types_.end();
}

void TypeController::declareType(TypeId id) {
    ensureMutable("TypeController::declareType");
    requireTypeId(id, "TypeController::declareType: type id cannot be empty");
    if (!hasConcreteType(id) && !hasAlias(id))
        declarations_.insert(std::move(id));
}

bool TypeController::isDeclared(const TypeId &id) const {
    if (!id.valid())
        return false;
    const TypeId resolved{canonical(id)};
    return hasConcreteType(resolved) || declarations_.find(resolved) != declarations_.end();
}

registry::RegisterStatus TypeController::registerType(std::unique_ptr<TypeDefinition> type) {
    ensureMutable("TypeController::registerType");
    if (!type)
        throw std::runtime_error("TypeController::registerType: type cannot be null");
    validateTypeDefinition(*type);

    const TypeId id{type->id};
    if (hasAlias(id))
        throw std::runtime_error("TypeController::registerType: name '" + id.name + "' is already registered as a type alias");

    auto found{types_.find(id)};
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

bool TypeController::hasType(const TypeId &id) const {
    return findType(id) != nullptr;
}

const TypeDefinition *TypeController::findType(const TypeId &id) const {
    if (!id.valid())
        return nullptr;
    const TypeId resolved{canonical(id)};
    const auto it{types_.find(resolved)};
    return it == types_.end() ? nullptr : it->second.get();
}

const TypeDefinition &TypeController::requireType(const TypeId &id) const {
    const TypeDefinition *type{findType(id)};
    if (!type)
        throw std::runtime_error("TypeController::requireType: unknown type '" + id.name + "'");
    return *type;
}

const std::vector<TypeId> &TypeController::typeIds() const noexcept {
    return typeIds_;
}

size_t TypeController::typeCount() const noexcept {
    return types_.size();
}

// Aliases

bool TypeController::hasAlias(const TypeId &id) const noexcept {
    return aliases_.find(id) != aliases_.end();
}

std::optional<TypeId> TypeController::aliasTarget(const TypeId &id) const {
    const auto found{aliases_.find(id)};
    if (found == aliases_.end())
        return std::nullopt;
    return found->second;
}

bool TypeController::wouldCreateAliasCycle(const TypeId &alias, const TypeId &target) const {
    TypeId current = target;
    std::unordered_set<TypeId, TypeIdHash> visited{};
    visited.insert(alias);

    while (true) {
        if (!visited.insert(current).second) {
            return true;
        }
        const auto found = aliases_.find(current);
        if (found == aliases_.end()) {
            return false;
        }
        current = found->second;
    }
}

registry::RegisterStatus TypeController::registerAlias(TypeId alias, TypeId target) {
    ensureMutable("TypeController::registerAlias");
    requireTypeId(alias, "TypeController::registerAlias: alias cannot be empty");
    requireTypeId(target, "TypeController::registerAlias: target cannot be empty");

    if (hasConcreteType(alias)) {
        throw std::runtime_error(
            "TypeController::registerAlias: name '" + alias.name + "' is already a concrete type"
        );
    }
    if (!isDeclared(target)) {
        throw std::runtime_error(
            "TypeController::registerAlias: target type '" + target.name + "' is neither registered nor declared"
        );
    }
    if (wouldCreateAliasCycle(alias, target)) {
        throw std::runtime_error(
            "TypeController::registerAlias: alias '" + alias.name + "' would create a cycle"
        );
    }

    const auto found = aliases_.find(alias);
    if (found == aliases_.end()) {
        aliases_.emplace(alias, std::move(target));
        declarations_.erase(alias);
        return registry::RegisterStatus::Inserted;
    }

    switch (options_.duplicatePolicy) {
        case registry::DuplicatePolicy::Error:
            throw std::runtime_error("TypeController::registerAlias: duplicate alias '" + alias.name + "'");
        case registry::DuplicatePolicy::Ignore:
            return registry::RegisterStatus::Ignored;
        case registry::DuplicatePolicy::Replace:
            found->second = std::move(target);
            declarations_.erase(alias);
            return registry::RegisterStatus::Replaced;
    }
    throw std::runtime_error("TypeController::registerAlias: unknown duplicate policy");
}

TypeId TypeController::canonical(TypeId id) const {
    requireTypeId(id, "TypeController::canonical: type id cannot be empty");
    std::unordered_set<TypeId, TypeIdHash> visited{};

    while (true) {
        if (!visited.insert(id).second)
            throw std::runtime_error("TypeController::canonical: cyclic alias involving '" + id.name + "'");
        const auto found{aliases_.find(id)};
        if (found == aliases_.end()) {
            return id;
        }
        id = found->second;
    }
}

bool TypeController::equivalent(const TypeId &left, const TypeId &right) const {
    if (!isDeclared(left) || !isDeclared(right))
        return false;
    return canonical(left) == canonical(right);
}

// Primitive helpers

TypeController::PrimitiveBuilder TypeController::definePrimitive(TypeId id) {
    ensureMutable("TypeController::definePrimitive");
    return PrimitiveBuilder{*this, std::move(id)};
}

registry::RegisterStatus TypeController::registerPrimitive(PrimitiveType type) {
    return commitPrimitive(std::move(type), {});
}

registry::RegisterStatus TypeController::commitPrimitive(PrimitiveType type, std::unordered_map<TypeId, TypeConversion, TypeIdHash> conversions) {
    ensureMutable("TypeController::PrimitiveBuilder::commit");
    if (type.storageBits == 0) {
        type.storageBits = type.bitWidth;
    }
    validatePrimitive(type);

    std::unordered_map<TypeId, TypeConversion, TypeIdHash> normalized{};
    for (auto &[target, conversion] : conversions) {
        if (!(target == conversion.target)) {
            throw std::runtime_error("TypeController::PrimitiveBuilder: conversion key does not match target");
        }
        
        TypeId canonicalTarget = canonical(target);
        if (!(canonicalTarget == type.id) && !isDeclared(canonicalTarget))
            throw std::runtime_error("TypeController::PrimitiveBuilder: conversion target '" + target.name + "' is neither registered nor declared");
        
        conversion.target = canonicalTarget;
        if (conversion.kind == ConversionKind::Implicit && conversion.rank == 0)
            throw std::runtime_error("TypeController::PrimitiveBuilder: implicit conversion rank must be greater than zero");

        const auto existingPending{normalized.find(canonicalTarget)};
        if (existingPending != normalized.end()) {
            switch (options_.duplicatePolicy) {
                case registry::DuplicatePolicy::Error:
                    throw std::runtime_error("TypeController::PrimitiveBuilder: multiple conversions resolve to target '" + canonicalTarget.name + "'");
                case registry::DuplicatePolicy::Ignore:
                    continue;
                case registry::DuplicatePolicy::Replace:
                    existingPending->second = std::move(conversion);
                    continue;
            }
        }

        if (findConversion(type.id, canonicalTarget) != nullptr && options_.duplicatePolicy == registry::DuplicatePolicy::Error)
            throw std::runtime_error("TypeController::PrimitiveBuilder: duplicate conversion from '" + type.id.name + "' to '" + canonicalTarget.name + "'");
        normalized.emplace(canonicalTarget, std::move(conversion));
    }

    const TypeId id{type.id};
    const registry::RegisterStatus status{registerType(std::make_unique<PrimitiveType>(std::move(type)))};
    if (status == registry::RegisterStatus::Ignored) {
        return status;
    }

    // Conversions are independent type relations. Replacing a type does not
    // silently delete unrelated conversions; only supplied targets are updated.
    for (auto &[target, conversion] : normalized) {
        if (options_.duplicatePolicy == registry::DuplicatePolicy::Ignore && findConversion(id, target) != nullptr)
            continue;
        conversion.extensions.freeze();
        conversions_[id].insert_or_assign(target, std::move(conversion));
    }
    return status;
}

bool TypeController::hasPrimitive(const TypeId &id) const {
    return findPrimitive(id) != nullptr;
}

const PrimitiveType *TypeController::findPrimitive(const TypeId &id) const {
    return dynamic_cast<const PrimitiveType *>(findType(id));
}

const PrimitiveType &TypeController::requirePrimitive(const TypeId &id) const {
    const PrimitiveType *type{findPrimitive(id)};
    if (!type)
        throw std::runtime_error("TypeController::requirePrimitive: unknown primitive type '" + id.name + "'");
    return *type;
}

std::vector<TypeId> TypeController::primitiveIds() const {
    std::vector<TypeId> result;
    result.reserve(types_.size());
    for (const TypeId &id : typeIds_)
        if (hasPrimitive(id))
            result.push_back(id);
    return result;
}

size_t TypeController::primitiveCount() const noexcept {
    size_t count{};
    for (const auto &[_, type] : types_) {
        if (dynamic_cast<const PrimitiveType *>(type.get()) != nullptr) {
            ++count;
        }
    }
    return count;
}

// Type relations

registry::RegisterStatus TypeController::registerConversion(TypeId source, TypeId target, ConversionKind kind, size_t rank, ExtensionSet extensions) {
    ensureMutable("TypeController::registerConversion");
    requireTypeId(source, "TypeController::registerConversion: source type cannot be empty");
    requireTypeId(target, "TypeController::registerConversion: target type cannot be empty");

    source = canonical(std::move(source));
    target = canonical(std::move(target));
    TypeConversion conversion{target, kind, rank, std::move(extensions)};
    validateConversion(source, conversion);
    conversion.extensions.freeze();

    auto &bucket{conversions_[source]};
    const auto found{bucket.find(target)};
    if (found == bucket.end()) {
        bucket.emplace(target, std::move(conversion));
        return registry::RegisterStatus::Inserted;
    }

    switch (options_.duplicatePolicy) {
        case registry::DuplicatePolicy::Error:
            throw std::runtime_error("TypeController::registerConversion: duplicate conversion from '" + source.name + "' to '" + target.name + "'");
        case registry::DuplicatePolicy::Ignore:
            return registry::RegisterStatus::Ignored;
        case registry::DuplicatePolicy::Replace:
            found->second = std::move(conversion);
            return registry::RegisterStatus::Replaced;
    }
    throw std::runtime_error("TypeController::registerConversion: unknown duplicate policy");
}

const TypeConversion *TypeController::findConversion(const TypeId &source, const TypeId &target) const {
    if (!source.valid() || !target.valid())
        return nullptr;
    const TypeId canonicalSource{canonical(source)};
    const TypeId canonicalTarget{canonical(target)};
    const auto sourceIt{conversions_.find(canonicalSource)};
    if (sourceIt == conversions_.end()) {
        return nullptr;
    }
    const auto targetIt{sourceIt->second.find(canonicalTarget)};
    return targetIt == sourceIt->second.end() ? nullptr : &targetIt->second;
}

bool TypeController::canImplicitlyConvert(const TypeId &source, const TypeId &target) const {
    if (!source.valid() || !target.valid()) return false;
    if (equivalent(source, target)) return true;

    const TypeConversion *conversion{findConversion(source, target)};
    return conversion && conversion->isImplicit();
}

void TypeController::validate() const {
    if (!declarations_.empty()) {
        const auto &id{*declarations_.begin()};
        throw std::runtime_error("TypeController::validate: unresolved declared type '" + id.name + "'");
    }

    for (const auto &[alias, _] : aliases_) {
        const TypeId target{canonical(alias)};
        if (!hasConcreteType(target))
            throw std::runtime_error("TypeController::validate: alias '" + alias.name + "' resolves to unregistered type '" + target.name + "'");
    }

    for (const auto &[source, targets] : conversions_) {
        if (!hasConcreteType(source))
            throw std::runtime_error("TypeController::validate: conversion source '" + source.name + "' is not registered");

        for (const auto &[target, _] : targets)
            if (!hasConcreteType(target))
                throw std::runtime_error("TypeController::validate: conversion target '" + target.name + "' is not registered");
    }
}

void TypeController::finalize() {
    if (finalized_)
        return;
    validate();
    finalized_ = true;
}

// Literal semantics

registry::RegisterStatus TypeController::bindLiteral(const atomic::LiteralFeature &literal, TypeId type) {
    return bindLiteral(literal.info(), std::move(type));
}

registry::RegisterStatus TypeController::bindLiteral(const atomic::LiteralInfo &literal, TypeId type) {
    ensureMutable("TypeController::bindLiteral");
    type = canonical(std::move(type));
    if (!hasConcreteType(type))
        throw std::runtime_error("TypeController::bindLiteral: unknown literal type '" + type.name + "'");

    return bindLiteral(literal, [type = std::move(type)](const ast::Node &) -> std::optional<TypeId> {
        return type;
    });
}

registry::RegisterStatus TypeController::bindLiteral(const atomic::LiteralFeature &literal, LiteralTypeResolver resolver) {
    return bindLiteral(literal.info(), std::move(resolver));
}

registry::RegisterStatus TypeController::bindLiteral(const atomic::LiteralInfo &literal, LiteralTypeResolver resolver) {
    ensureMutable("TypeController::bindLiteral");
    validateLiteralInfo(literal);
    if (!resolver)
        throw std::runtime_error("TypeController::bindLiteral: resolver cannot be empty");

    LiteralBinding binding{literal.id, literal.nodeKind, std::move(resolver)};
    const auto found{literalBindingsByFeature_.find(literal.id)};
    if (found == literalBindingsByFeature_.end()) {
        literalBindingsByFeature_.emplace(literal.id, std::move(binding));
        literalFeaturesByNodeKind_[literal.nodeKind].push_back(literal.id);
        return registry::RegisterStatus::Inserted;
    }

    switch (options_.duplicatePolicy) {
        case registry::DuplicatePolicy::Error:
            throw std::runtime_error("TypeController::bindLiteral: literal feature '" + literal.id + "' already has typing semantics");
        case registry::DuplicatePolicy::Ignore:
            return registry::RegisterStatus::Ignored;
        case registry::DuplicatePolicy::Replace: {
            const std::string oldKind{found->second.nodeKind};
            found->second = std::move(binding);
            if (oldKind != literal.nodeKind) {
                auto &oldList{literalFeaturesByNodeKind_[oldKind]};
                oldList.erase(std::remove(oldList.begin(), oldList.end(), literal.id), oldList.end());
                auto &newList{literalFeaturesByNodeKind_[literal.nodeKind]};
                if (std::find(newList.begin(), newList.end(), literal.id) == newList.end())
                    newList.push_back(literal.id);
            }
            return registry::RegisterStatus::Replaced;
        }
    }

    throw std::runtime_error("TypeController::bindLiteral: unknown duplicate policy");
}

std::optional<TypeId> TypeController::resolveLiteralBinding(const LiteralBinding &binding, const ast::Node &node) const {
    std::optional<TypeId> result = binding.resolver(node);
    if (!result)
        return std::nullopt;

    *result = canonical(std::move(*result));
    if (!hasConcreteType(*result))
        throw std::runtime_error("TypeController::resolveLiteral: resolver for literal feature '" + binding.featureId + "' returned unknown type '" + result->name + "'");

    return result;
}

std::optional<TypeId> TypeController::resolveLiteral(const atomic::LiteralFeature &literal, const ast::Node &node) const {
    return resolveLiteral(literal.info(), node);
}

std::optional<TypeId> TypeController::resolveLiteral(const atomic::LiteralInfo &literal, const ast::Node &node) const {
    validateLiteralInfo(literal);
    const auto found{literalBindingsByFeature_.find(literal.id)};
    if (found == literalBindingsByFeature_.end()) return std::nullopt;
    if (found->second.nodeKind != node.kind()) return std::nullopt;

    return resolveLiteralBinding(found->second, node);
}

std::optional<TypeId> TypeController::resolveLiteral(const ast::Node &node) const {
    const auto byKind{literalFeaturesByNodeKind_.find(node.kind())};
    if (byKind == literalFeaturesByNodeKind_.end())
        return std::nullopt;

    std::optional<TypeId> resolved;
    for (const std::string &featureId : byKind->second) {
        const auto binding{literalBindingsByFeature_.find(featureId)};
        if (binding == literalBindingsByFeature_.end())
            continue;

        std::optional<TypeId> candidate = resolveLiteralBinding(binding->second, node);
        if (!candidate)
            continue;

        if (!resolved)
            resolved = std::move(candidate);
        
        else if (!equivalent(*resolved, *candidate))
            throw std::runtime_error("TypeController::resolveLiteral: AST node kind '" + node.kind() + "' matches multiple literal features with different types; resolve using the LiteralFeature identity");
    }
    return resolved;
}

// Operation semantics

registry::RegisterStatus TypeController::registerOperation(const atomic::OperationFeature &operation, std::vector<TypeId> operands, TypeId result, ExtensionSet extensions) {
    return registerOperation(operation.info(), OperationSignature{std::move(operands), std::move(result), std::move(extensions)});
}

registry::RegisterStatus TypeController::registerOperation(const atomic::OperationInfo &operation, OperationSignature signature) {
    ensureMutable("TypeController::registerOperation");
    validateOperationInfo(operation);
    for (TypeId &operand : signature.operands)
        operand = canonical(std::move(operand));

    signature.result = canonical(std::move(signature.result));
    validateSignature(operation, signature);
    signature.extensions.freeze();

    auto &bucket{operations_[operation.id]};
    const auto existing = std::find_if(bucket.begin(), bucket.end(),[&](const std::shared_ptr<const OperationSignature> &candidate) {
            return sameOperands(*candidate, signature);
        }
    );

    auto stored{std::make_shared<const OperationSignature>(std::move(signature))};
    if (existing == bucket.end()) {
        bucket.push_back(std::move(stored));
        return registry::RegisterStatus::Inserted;
    }

    switch (options_.duplicatePolicy) {
        case registry::DuplicatePolicy::Error:
            throw std::runtime_error("TypeController::registerOperation: duplicate typed overload for operation '" + operation.id + "'");
        case registry::DuplicatePolicy::Ignore:
            return registry::RegisterStatus::Ignored;
        case registry::DuplicatePolicy::Replace:
            *existing = std::move(stored);
            return registry::RegisterStatus::Replaced;
    }

    throw std::runtime_error("TypeController::registerOperation: unknown duplicate policy");
}

std::optional<OperationResolution> TypeController::resolveOperation(const atomic::OperationFeature &operation, std::span<const TypeId> operands) const {
    return resolveOperation(operation.info(), operands);
}

std::optional<OperationResolution> TypeController::resolveOperation(const atomic::OperationInfo &operation, std::span<const TypeId> operands) const {
    OperationResolutionResult result = resolveOperationDetailed(operation, operands);
    return result.ok() ? result.resolution : std::nullopt;
}

OperationResolutionResult TypeController::resolveOperationDetailed(const atomic::OperationFeature &operation, std::span<const TypeId> operands) const {
    return resolveOperationDetailed(operation.info(), operands);
}

OperationResolutionResult TypeController::resolveOperationDetailed(const atomic::OperationInfo &operation, std::span<const TypeId> operands) const {
    validateOperationInfo(operation);
    const size_t arity{expectedArity(operation)};
    if (operands.size() != arity) {
        return OperationResolutionResult::invalidArity("operation '" + operation.id + "' expects " + std::to_string(arity) + " operand(s), got " + std::to_string(operands.size()));
    }

    std::vector<TypeId> canonicalOperands;
    canonicalOperands.reserve(operands.size());
    for (const TypeId &operand : operands) {
        if (!operand.valid())
            return OperationResolutionResult::unknownOperand("operation '" + operation.id + "' received an empty operand type");

        const TypeId resolved{canonical(operand)};
        if (!hasConcreteType(resolved))
            return OperationResolutionResult::unknownOperand("operation '" + operation.id + "' received unknown operand type '" + operand.name + "'");

        canonicalOperands.push_back(resolved);
    }

    OperationResolutionResult fixed = resolveRegisteredOperations(operation.id, canonicalOperands);
    if (fixed.status == OperationResolutionStatus::Resolved ||
        fixed.status == OperationResolutionStatus::Ambiguous ||
        fixed.status == OperationResolutionStatus::InvalidConfiguration) {
        return fixed;
    }
    return resolveCustomRules(operation.id, canonicalOperands);
}

void TypeController::registerSemanticRule(const atomic::OperationFeature &operation, std::unique_ptr<OperationSemanticRule> rule) {
    registerSemanticRule(operation.info(), std::move(rule));
}

void TypeController::registerSemanticRule(const atomic::OperationInfo &operation, std::unique_ptr<OperationSemanticRule> rule) {
    ensureMutable("TypeController::registerSemanticRule");
    validateOperationInfo(operation);
    if (!rule)
        throw std::runtime_error("TypeController::registerSemanticRule: rule cannot be null");

    std::shared_ptr<const OperationSemanticRule> stored{std::move(rule)};
    semanticRules_[operation.id].push_back(std::move(stored));
}

OperationResolutionResult TypeController::resolveRegisteredOperations(const std::string &operationId, std::span<const TypeId> operands) const {
    const auto bucket = operations_.find(operationId);
    if (bucket == operations_.end())
        return OperationResolutionResult::noMatch("no typed overload is registered for operation '" + operationId + "'");

    std::shared_ptr<const OperationSignature> best{};
    std::vector<ConversionStep> bestConversions{};
    size_t bestCost{std::numeric_limits<size_t>::max()};
    std::vector<OperationCandidateDiagnostic> bestCandidates{};

    for (const auto &signature : bucket->second) {
        if (signature->operands.size() != operands.size())
            continue;

        std::vector<ConversionStep> conversions{};
        size_t cost{};
        bool viable{true};

        for (size_t index{}; index < operands.size(); ++index) {
            if (equivalent(operands[index], signature->operands[index])) {
                continue;
            }

            const TypeConversion *conversion{findConversion(operands[index], signature->operands[index])};
            if (!conversion || !conversion->isImplicit()) {
                viable = false;
                break;
            }

            if (conversion->rank > std::numeric_limits<size_t>::max() - cost) {
                viable = false;
                break;
            }
            cost += conversion->rank;
            conversions.push_back(ConversionStep{index, operands[index], signature->operands[index], *conversion});
        }

        if (!viable) {
            continue;
        }

        OperationCandidateDiagnostic diagnostic{signature->operands, signature->result, cost};
        if (cost < bestCost) {
            best = signature;
            bestConversions = std::move(conversions);
            bestCost = cost;
            bestCandidates.clear();
            bestCandidates.push_back(std::move(diagnostic));
        } else if (cost == bestCost) {
            bestCandidates.push_back(std::move(diagnostic));
        }
    }

    if (bestCandidates.size() > 1)
        return OperationResolutionResult::ambiguous("ambiguous overload for operation '" + operationId + "' with operands " + formatTypeList(operands), std::move(bestCandidates));

    if (!best)
        return OperationResolutionResult::noMatch("no matching overload for operation '" + operationId + "' with operands " + formatTypeList(operands));


    OperationResolution result;
    result.result = best->result;
    result.conversions = std::move(bestConversions);
    result.signature = best;
    result.extensions = best->extensions;
    return OperationResolutionResult::resolved(std::move(result));
}

OperationResolutionResult TypeController::resolveCustomRules(const std::string &operationId, std::span<const TypeId> operands) const {
    const auto bucket{semanticRules_.find(operationId)};
    if (bucket == semanticRules_.end())
        return OperationResolutionResult::noMatch("no semantic rule matched operation '" + operationId + "' with operands " + formatTypeList(operands));


    int bestPriority{std::numeric_limits<int>::min()};
    std::optional<OperationResolution> best{};
    size_t bestRuleCount{};

    for (const auto &rule : bucket->second) {
        try {
            std::optional<SemanticOperationResolution> semantic = rule->resolve(*this, operands);
            if (!semantic)
                continue;

            OperationResolution candidate{materializeSemanticResolution(operands, std::move(*semantic))};

            if (!best || rule->priority() > bestPriority) {
                bestPriority = rule->priority();
                best = std::move(candidate);
                bestRuleCount = 1;
            } 
            else if (rule->priority() == bestPriority)
                ++bestRuleCount;

        } catch (const std::exception &error) {
            return OperationResolutionResult::invalidConfiguration("semantic rule for operation '" + operationId + "' is invalid: " + error.what());
        }
    }

    if (bestRuleCount > 1)
        return OperationResolutionResult::ambiguous("multiple semantic rules with priority " + std::to_string(bestPriority) +" match operation '" + operationId + "'");

    if (!best)
        return OperationResolutionResult::noMatch("no semantic rule matched operation '" + operationId + "' with operands " + formatTypeList(operands));

    return OperationResolutionResult::resolved(std::move(*best));
}

registry::DuplicatePolicy TypeController::duplicatePolicy() const noexcept {
    return options_.duplicatePolicy;
}

// Validation

void TypeController::validateTypeDefinition(const TypeDefinition &type) {
    requireTypeId(type.id, "TypeController::registerType: type id cannot be empty");
}

void TypeController::validatePrimitive(const PrimitiveType &type) const {
    validateTypeDefinition(type);
    if (hasAlias(type.id))
        throw std::runtime_error("TypeController::registerPrimitive: name '" + type.id.name + "' is already a type alias");

    if (type.bitWidth == 0)
        throw std::runtime_error("TypeController::registerPrimitive: primitive '" + type.id.name + "' must use at least one bit");

    if (type.storageBits < type.bitWidth)
        throw std::runtime_error("TypeController::registerPrimitive: storage width for '" + type.id.name +"' cannot be smaller than its semantic bit width");

    if (type.alignment == 0)
        throw std::runtime_error("TypeController::registerPrimitive: alignment for '" + type.id.name + "' must be non-zero");

    if (!type.representation)
        throw std::runtime_error("TypeController::registerPrimitive: primitive representation cannot be null");
}

void TypeController::validateConversion(const TypeId &source, const TypeConversion &conversion) const {
    requireTypeId(source, "TypeController::registerConversion: source type cannot be empty");
    requireTypeId(conversion.target, "TypeController::registerConversion: target type cannot be empty");
    if (!isDeclared(source))
        throw std::runtime_error("TypeController::registerConversion: source type '" + source.name + "' is neither registered nor declared");

    if (!isDeclared(conversion.target))
        throw std::runtime_error("TypeController::registerConversion: target type '" + conversion.target.name + "' is neither registered nor declared");

    if (conversion.kind == ConversionKind::Implicit && conversion.rank == 0)
        throw std::runtime_error("TypeController::registerConversion: implicit conversion rank must be greater than zero");
}

void TypeController::validateLiteralInfo(const atomic::LiteralInfo &literal) {
    if (literal.id.empty())
        throw std::runtime_error("TypeController::bindLiteral: literal id cannot be empty");
    
    if (literal.nodeKind.empty())
        throw std::runtime_error("TypeController::bindLiteral: literal node kind cannot be empty");
}

void TypeController::validateOperationInfo(const atomic::OperationInfo &operation) {
    if (operation.id.empty())
        throw std::runtime_error("TypeController: operation id cannot be empty");
}

size_t TypeController::expectedArity(const atomic::OperationInfo &operation) noexcept {
    switch (operation.arity) {
        case atomic::OperationArity::Unary:
        case atomic::OperationArity::Postfix:
            return 1;
        case atomic::OperationArity::Binary:
            return 2;
    }
    return 0;
}

void TypeController::validateSignature(const atomic::OperationInfo &operation, const OperationSignature &signature) const {
    const size_t arity{expectedArity(operation)};
    if (signature.operands.size() != arity) {
        throw std::runtime_error("TypeController::registerOperation: operation '" + operation.id + "' expects " + std::to_string(arity) + " operand type(s), got " + std::to_string(signature.operands.size()));
    }
    
    for (const TypeId &operand : signature.operands) {
        requireTypeId(operand, "TypeController::registerOperation: operand type cannot be empty");
        if (!hasConcreteType(operand))
            throw std::runtime_error("TypeController::registerOperation: unknown operand type '" + operand.name + "'");
    }
    requireTypeId(signature.result, "TypeController::registerOperation: result type cannot be empty");
    
    if (!hasConcreteType(signature.result))
        throw std::runtime_error("TypeController::registerOperation: unknown result type '" + signature.result.name + "'");
}

OperationResolution TypeController::materializeSemanticResolution(std::span<const TypeId> operands, SemanticOperationResolution semantic) const {
    requireTypeId(semantic.result, "TypeController::resolveOperation: semantic rule returned an empty result type");
    semantic.result = canonical(std::move(semantic.result));
    
    if (!hasConcreteType(semantic.result))
        throw std::runtime_error("TypeController::resolveOperation: semantic rule returned unknown result type '" + semantic.result.name + "'");

    OperationResolution result;
    result.result = std::move(semantic.result);
    result.extensions = std::move(semantic.extensions);
    result.extensions.freeze();

    std::vector<bool> converted(operands.size(), false);
    for (SemanticConversionRequest request : semantic.conversions) {
        if (request.operandIndex >= operands.size())
            throw std::runtime_error("semantic rule returned an out-of-range operand index");

        if (converted[request.operandIndex])
            throw std::runtime_error("semantic rule requested multiple conversions for one operand");

        converted[request.operandIndex] = true;
        requireTypeId(request.target, "semantic rule returned an empty conversion target");
        request.target = canonical(std::move(request.target));

        const TypeId &source{operands[request.operandIndex]};
        if (equivalent(source, request.target))
            continue;

        const TypeConversion *registered{findConversion(source, request.target)};
        if (!registered || !registered->isImplicit())
            throw std::runtime_error("semantic rule requested conversion from '" + source.name + "' to '" + request.target.name + "', but no direct implicit conversion is registered");

        result.conversions.push_back(ConversionStep{request.operandIndex, source, request.target, *registered});
    }
    return result;
}

} // namespace novac::assets::types
