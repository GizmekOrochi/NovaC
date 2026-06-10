#pragma once

#include "TypeSystem.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace novac::traits {

struct TraitMethod {
    std::string name{};
    std::vector<std::string> parameterTypes{};
    std::string returnType{};
    bool hasDefaultImplementation{false};
};

struct AssociatedType {
    std::string name{};
    std::string constraint{};
    bool required{true};
};

struct WhereClause {
    std::string typeVariable{};
    std::string traitName{};
};

struct Trait {
    std::string name{};
    std::vector<std::string> requiredOperators{};
    std::vector<TraitMethod> methods{};
    std::vector<AssociatedType> associatedTypes{};
    std::vector<WhereClause> whereClauses{};
};

struct Constraint {
    std::string typeVariable{};
    std::string traitName{};
};

struct TraitImplementation {
    std::string traitName{};
    std::string typeName{};
    std::vector<TraitMethod> methods{};
    std::unordered_map<std::string, std::string> associatedTypes{};
};

class TraitRegistry {
public:
    void addTrait(Trait trait);

    const Trait *find(const std::string &name) const;

    void impl(const std::string &trait, const std::string &typeName);
    void addImplementation(TraitImplementation implementation);

    bool satisfies(
        const std::string &trait,
        types::TypeRef type) const;

    bool hasMethod(
        const std::string &trait,
        const std::string &method) const;

    void validateImplementation(
        const TraitImplementation &implementation) const;

private:
    std::unordered_map<std::string, Trait> traits_;
    std::unordered_map<
        std::string,
        std::unordered_map<std::string, TraitImplementation>>
        implementations_;
};

class ConstraintSolver {
public:
    explicit ConstraintSolver(const TraitRegistry &traits);

    bool solve(
        const std::vector<Constraint> &constraints,
        const types::Substitution &substitution) const;

private:
    const TraitRegistry &traits_;
};

} // namespace novac::traits