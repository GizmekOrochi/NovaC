#pragma once

#include "../syntax/Node.hpp"
#include "Traits.hpp"
#include "TypeSystem.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace novac::templates {

struct TemplateParameter {
    std::string name{};
    std::vector<traits::Constraint> constraints{};
};

struct GenericDeclaration {
    std::string name{};
    std::vector<TemplateParameter> parameters{};
    ast::NodePtr body{};
};

struct SpecializationSymbol {
    std::string genericName{};
    std::string mangledName{};
    std::vector<types::TypeRef> arguments{};
    ast::NodePtr specializedBody{};
};

class TemplateRegistry {
public:
    void add(GenericDeclaration declaration);

    const GenericDeclaration *find(const std::string &name) const;

private:
    std::unordered_map<std::string, GenericDeclaration> generics_;
};

class SpecializationRegistry {
public:
    bool add(SpecializationSymbol symbol);

    const SpecializationSymbol *findMangled(const std::string &mangled) const;

    const std::unordered_map<std::string, SpecializationSymbol> &all() const;

private:
    std::unordered_map<std::string, SpecializationSymbol> symbols_;
};

class InstantiationCache {
public:
    ast::NodePtr find(const std::string &key) const;

    void remember(std::string key, ast::NodePtr node);

private:
    std::unordered_map<std::string, ast::NodePtr> cache_;
};

class ASTCloner {
public:
    ast::NodePtr clone(
        const ast::NodePtr &node,
        const types::Substitution &substitution = {}) const;

private:
    ast::Field cloneField(
        const std::string &name,
        const ast::Field &field,
        const types::Substitution &substitution) const;
};

class InstantiationEngine {
public:
    InstantiationEngine(
        const TemplateRegistry &registry,
        InstantiationCache &cache,
        SpecializationRegistry &specializations,
        const traits::TraitRegistry &traits);

    ast::NodePtr instantiate(
        const std::string &name,
        const std::vector<types::TypeRef> &args);

    static std::string makeKey(
        const std::string &name,
        const std::vector<types::TypeRef> &args);

    static std::string mangle(
        const std::string &name,
        const std::vector<types::TypeRef> &args);

private:
    const TemplateRegistry &registry_;
    InstantiationCache &cache_;
    SpecializationRegistry &specializations_;
    const traits::TraitRegistry &traits_;
};

} // namespace novac::templates