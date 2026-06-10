#pragma once

#include "../../engine/syntax/Node.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace novac::types {

struct Type;

using TypeRef = std::shared_ptr<const Type>;
using Substitution = std::unordered_map<std::string, TypeRef>;

enum class TypeKind {
    Primitive,
    Function,
    Array,
    Struct,
    Generic,
    Variable,
    Trait,
    Unknown,
    Void
};

struct Type {
    TypeKind kind{TypeKind::Unknown};
    std::string name{};
    std::vector<TypeRef> args{};
    std::vector<TypeRef> params{};
    TypeRef result{};
    std::unordered_map<std::string, TypeRef> fields{};

    std::string display() const;
};

struct ConversionRule {
    TypeRef from{};
    TypeRef to{};
    int rank{};
    bool implicit{true};
};

class ConversionRegistry {
public:
    void add(
        TypeRef from,
        TypeRef to,
        int rank = 1,
        bool implicit = true);

    const ConversionRule *find(
        TypeRef from,
        TypeRef to,
        bool implicitOnly = true) const;

private:
    std::vector<ConversionRule> rules_;
};

class TypeUnifier {
public:
    bool unify(
        TypeRef pattern,
        TypeRef actual,
        Substitution &substitution) const;
};

class TypeRegistry {
public:
    TypeRegistry();

    TypeRef primitive(const std::string &name);
    TypeRef variable(const std::string &name);
    TypeRef generic(const std::string &base, std::vector<TypeRef> args);
    TypeRef function(std::vector<TypeRef> params, TypeRef result);
    TypeRef array(TypeRef element);

    TypeRef voidType() const;
    TypeRef unknown() const;
    TypeRef find(const std::string &name) const;

    ConversionRegistry &conversions();
    const ConversionRegistry &conversions() const;

    bool assignable(TypeRef expected, TypeRef actual) const;

private:
    std::unordered_map<std::string, TypeRef> named_;
    ConversionRegistry conversions_;
    TypeRef void_;
    TypeRef unknown_;
};

} // namespace novac::types