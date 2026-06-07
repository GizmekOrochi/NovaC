#include "../../include/types/TypeSystem.hpp"

#include <utility>

namespace novac::types {

std::string Type::display() const {
    if (kind == TypeKind::Function) {
        std::string output{"fn("};

        for (std::size_t index{}; index < params.size(); ++index) {
            if (index != 0) {
                output += ",";
            }

            output += params[index]->display();
        }

        output += ")->";

        if (result) {
            output += result->display();
        } else {
            output += "void";
        }

        return output;
    }

    if (args.empty()) {
        return name;
    }

    std::string output{name + "<"};

    for (std::size_t index{}; index < args.size(); ++index) {
        if (index != 0) {
            output += ",";
        }

        output += args[index]->display();
    }

    output += ">";

    return output;
}

void ConversionRegistry::add(
    TypeRef from,
    TypeRef to,
    int rank,
    bool implicit) {
    rules_.push_back({
        std::move(from),
        std::move(to),
        rank,
        implicit
    });
}

const ConversionRule *ConversionRegistry::find(
    TypeRef from,
    TypeRef to,
    bool implicitOnly) const {
    const ConversionRule *best{nullptr};

    for (const ConversionRule &rule : rules_) {
        if (implicitOnly && !rule.implicit) {
            continue;
        }

        if (!rule.from || !rule.to || !from || !to) {
            continue;
        }

        if (rule.from->display() == from->display()
            && rule.to->display() == to->display()) {
            if (!best || rule.rank > best->rank) {
                best = &rule;
            }
        }
    }

    return best;
}

bool TypeUnifier::unify(
    TypeRef pattern,
    TypeRef actual,
    Substitution &substitution) const {
    if (!pattern || !actual) {
        return false;
    }

    if (pattern->kind == TypeKind::Variable) {
        const auto iter{substitution.find(pattern->name)};

        if (iter == substitution.end()) {
            substitution[pattern->name] = actual;

            return true;
        }

        return iter->second->display() == actual->display();
    }

    if (pattern->kind != actual->kind && pattern->kind != TypeKind::Generic) {
        return pattern->display() == actual->display();
    }

    if (pattern->name != actual->name || pattern->args.size() != actual->args.size()) {
        return pattern->display() == actual->display();
    }

    for (std::size_t index{}; index < pattern->args.size(); ++index) {
        if (!unify(pattern->args[index], actual->args[index], substitution)) {
            return false;
        }
    }

    return true;
}

TypeRegistry::TypeRegistry()
    : named_{}, conversions_{},
      void_{std::make_shared<Type>(Type{TypeKind::Void, "void", {}, {}, nullptr, {} })},
      unknown_{std::make_shared<Type>(Type{TypeKind::Unknown, "<unknown>", {}, {}, nullptr, {}})} {}

TypeRef TypeRegistry::primitive(const std::string &name) {
    return named_.try_emplace(name, std::make_shared<Type>(Type{TypeKind::Primitive, name, {}, {}, nullptr, {}})).first->second;
}

TypeRef TypeRegistry::variable(const std::string &name) {
    return std::make_shared<Type>(Type{TypeKind::Variable, name, {}, {}, nullptr, {}});
}

TypeRef TypeRegistry::generic(const std::string &base, std::vector<TypeRef> args) {
    return std::make_shared<Type>(Type{TypeKind::Generic, base, std::move(args), {}, nullptr, {}});
}

TypeRef TypeRegistry::function(std::vector<TypeRef> params, TypeRef result) {
    return std::make_shared<Type>(Type{TypeKind::Function, "fn", {}, std::move(params), std::move(result), {}});
}

TypeRef TypeRegistry::array(TypeRef element) {
    std::vector<TypeRef> args{};
    args.push_back(std::move(element));

    return generic("Array", std::move(args));
}

TypeRef TypeRegistry::voidType() const {
    return void_;
}

TypeRef TypeRegistry::unknown() const {
    return unknown_;
}

TypeRef TypeRegistry::find(const std::string &name) const {
    const auto iter{named_.find(name)};

    if (iter == named_.end()) {
        return nullptr;
    }

    return iter->second;
}

ConversionRegistry &TypeRegistry::conversions() {
    return conversions_;
}

const ConversionRegistry &TypeRegistry::conversions() const {
    return conversions_;
}

bool TypeRegistry::assignable(TypeRef expected, TypeRef actual) const {
    if (!expected || !actual) {
        return false;
    }

    if (expected->kind == TypeKind::Unknown || actual->kind == TypeKind::Unknown) {
        return true;
    }

    if (expected->display() == actual->display()) {
        return true;
    }

    if (expected->kind == TypeKind::Variable || actual->kind == TypeKind::Variable) {
        Substitution substitution{};

        return TypeUnifier{}.unify(expected, actual, substitution);
    }

    if (
        expected->kind == TypeKind::Generic
        && actual->kind == TypeKind::Generic
        && expected->name == actual->name
        && expected->args.size() == actual->args.size()) {
        for (std::size_t index{}; index < expected->args.size(); ++index) {
            if (!assignable(expected->args[index], actual->args[index])) {
                return false;
            }
        }

        return true;
    }

    return conversions_.find(actual, expected) != nullptr;
}

} // namespace novac::types