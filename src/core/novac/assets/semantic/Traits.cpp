#include "../../../../include/novac/assets/semantic/Traits.hpp"

#include <stdexcept>
#include <utility>

namespace novac::traits {

void TraitRegistry::addTrait(Trait trait) {
    const std::string name{trait.name};

    traits_[name] = std::move(trait);
}

const Trait *TraitRegistry::find(const std::string &name) const {
    const auto iter{traits_.find(name)};

    if (iter == traits_.end())
        return nullptr;

    return &iter->second;
}

void TraitRegistry::impl(const std::string &trait, const std::string &typeName) {
    addImplementation({trait, typeName, {}, {}});
}

void TraitRegistry::addImplementation(
    TraitImplementation implementation) {
    validateImplementation(implementation);

    implementations_[implementation.traitName][implementation.typeName] = std::move(implementation);
}

bool TraitRegistry::satisfies(const std::string &trait, types::TypeRef type) const {
    const auto traitIter{implementations_.find(trait)};

    if (traitIter == implementations_.end()) return false;

    if (!type) return false;

    return traitIter->second.find(type->display()) != traitIter->second.end();
}

bool TraitRegistry::hasMethod(const std::string &trait, const std::string &method) const {
    const Trait *traitDefinition{find(trait)};

    if (!traitDefinition)
        return false;

    for (const TraitMethod &traitMethod : traitDefinition->methods)
        if (traitMethod.name == method)
            return true;

    return false;
}

void TraitRegistry::validateImplementation(const TraitImplementation &implementation) const { 
    const Trait *traitDefinition{find(implementation.traitName)};

    if (!traitDefinition)
        throw std::runtime_error("TraitRegistry::validateImplementation: unknown trait implementation target '" + implementation.traitName + "'");

    for (const TraitMethod &requiredMethod : traitDefinition->methods) {
        if (requiredMethod.hasDefaultImplementation)
            continue;

        bool found{false};

        for (const TraitMethod &method : implementation.methods) {
            if (method.name == requiredMethod.name) {
                found = true;
                break;
            }
        }

        if (!found && !traitDefinition->requiredOperators.empty())
            found = true;

        if (!found) {
            throw std::runtime_error("TraitRegistry::validateImplementation: trait implementation missing method '" + requiredMethod.name + "' for trait '" + implementation.traitName + "'");
        }
    }

    for (const AssociatedType &associatedType : traitDefinition->associatedTypes) {
        if (!associatedType.required) {
            continue;
        }

        if (implementation.associatedTypes.find(associatedType.name) == implementation.associatedTypes.end()) {
            throw std::runtime_error("TraitRegistry::validateImplementation: trait implementation missing associated type '" + associatedType.name + "' for trait '" + implementation.traitName + "'");
        }
    }
}

ConstraintSolver::ConstraintSolver(const TraitRegistry &traits)
    : traits_{traits} {}

bool ConstraintSolver::solve(const std::vector<Constraint> &constraints, const types::Substitution &substitution) const {
    for (const Constraint &constraint : constraints) {
        const auto iter{substitution.find(constraint.typeVariable)};

        if (iter == substitution.end())
            return false;

        if (!traits_.satisfies(constraint.traitName, iter->second))
            return false;
    }

    return true;
}

} // namespace novac::traits