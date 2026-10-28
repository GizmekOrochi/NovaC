#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace novac::ir {

// Legacy primitive categories are still accepted by the public builders.
// TypeRef is the canonical, lossless identity stored in IR values.
enum class ValueType { Unknown, Void, Int, Bool, Float, String };

/** Stable, qualified identity; this is not a type definition or a type checker.
 * A language must allocate a distinct scope for semantically independent
 * type universes. The entire pair survives copying and serialization. */
struct TypeRef {
    std::string id{};
    std::string space{"novac.generic"};

    TypeRef() = default;
    TypeRef(std::string name, std::string scope = "novac.generic")
        : id{std::move(name)}, space{std::move(scope)} {}
    TypeRef(ValueType primitive) : space{"novac.builtin"} {
        switch (primitive) {
            case ValueType::Unknown: id = "unknown"; break;
            case ValueType::Void: id = "void"; break;
            case ValueType::Int: id = "int"; break;
            case ValueType::Bool: id = "bool"; break;
            case ValueType::Float: id = "float"; break;
            case ValueType::String: id = "string"; break;
        }
    }
    bool valid() const noexcept {
        if (space.empty() || id.empty()) return false;
        if (space != "novac.builtin") return true;
        return id == "unknown" || id == "void" || id == "int" ||
               id == "bool" || id == "float" || id == "string" || id == "dynamic";
    }
    bool builtin() const noexcept { return valid() && space == "novac.builtin"; }
    static TypeRef dynamic() { return {"dynamic", "novac.builtin"}; }
    bool unresolved() const noexcept { return *this == ValueType::Unknown; }
    bool isVoid() const noexcept { return *this == ValueType::Void; }
    std::string display() const { return builtin() ? id : space + "::" + id; }
    friend bool operator==(const TypeRef &, const TypeRef &) = default;
    friend bool operator==(const TypeRef &ref, ValueType primitive) {
        return ref == TypeRef{primitive};
    }
    // Length-prefixed form is unambiguous even if scope/id contain ':' or '\n'.
    std::string serialize() const {
        if (!valid()) throw std::runtime_error("TypeRef::serialize: invalid reference");
        return std::to_string(space.size()) + ":" + space +
               std::to_string(id.size()) + ":" + id;
    }
    static TypeRef deserialize(const std::string &encoded) {
        std::size_t position = 0;
        auto read = [&]() -> std::string {
            const auto colon = encoded.find(':', position);
            if (colon == std::string::npos || colon == position)
                throw std::runtime_error("TypeRef::deserialize: invalid length");
            std::size_t size = 0;
            for (std::size_t i = position; i < colon; ++i) {
                if (encoded[i] < '0' || encoded[i] > '9')
                    throw std::runtime_error("TypeRef::deserialize: invalid digit");
                const std::size_t digit = static_cast<std::size_t>(encoded[i] - '0');
                if (size > (std::numeric_limits<std::size_t>::max() - digit) / 10)
                    throw std::runtime_error("TypeRef::deserialize: length overflow");
                size = size * 10 + digit;
            }
            position = colon + 1;
            if (size > encoded.size() - position)
                throw std::runtime_error("TypeRef::deserialize: truncated reference");
            std::string value = encoded.substr(position, size);
            position += size;
            return value;
        };
        TypeRef result;
        result.space = read();
        result.id = read();
        if (!result.valid() || position != encoded.size())
            throw std::runtime_error("TypeRef::deserialize: invalid reference");
        return result;
    }
};

struct TypeRefHash {
    std::size_t operator()(const TypeRef &type) const noexcept {
        const auto a = std::hash<std::string>{}(type.space);
        const auto b = std::hash<std::string>{}(type.id);
        return a ^ (b + 0x9e3779b9U + (a << 6) + (a >> 2));
    }
};

/** Explicit scoped registry. Never depends on asset Types. */
class TypeSpace final {
public:
    explicit TypeSpace(std::string scope) : scope_{std::move(scope)} {
        if (scope_.empty() || scope_ == "novac.builtin")
            throw std::runtime_error("TypeSpace: invalid or reserved scope");
    }
    const std::string &scope() const noexcept { return scope_; }
    TypeRef intern(std::string stableKey) {
        if (stableKey.empty()) throw std::runtime_error("TypeSpace::intern: empty key");
        auto [it, _] = names_.insert(std::move(stableKey));
        return TypeRef{*it, scope_};
    }
    bool contains(const TypeRef &reference) const {
        return reference.space == scope_ && names_.contains(reference.id);
    }
    std::size_t size() const noexcept { return names_.size(); }
private:
    std::string scope_;
    std::unordered_set<std::string> names_;
};

/** Imports always require an explicit per-identity mapping between spaces. */
class TypeImportMap final {
public:
    void add(TypeRef source, TypeRef destination) {
        if (!source.valid() || !destination.valid())
            throw std::runtime_error("TypeImportMap::add: invalid reference");
        if (!entries_.emplace(std::move(source), std::move(destination)).second)
            throw std::runtime_error("TypeImportMap::add: duplicate source reference");
    }
    void add(const TypeSpace &sourceSpace, TypeRef source, const TypeSpace &destinationSpace, TypeRef destination) {
        if (!sourceSpace.contains(source) || !destinationSpace.contains(destination))
            throw std::runtime_error("TypeImportMap::add: reference not registered in declared scope");
        add(std::move(source), std::move(destination));
    }
    TypeRef resolve(const TypeRef &source) const {
        const auto it = entries_.find(source);
        if (it == entries_.end())
            throw std::runtime_error("TypeImportMap::resolve: missing explicit type import");
        return it->second;
    }
private:
    std::unordered_map<TypeRef, TypeRef, TypeRefHash> entries_{};
};

} // namespace novac::ir
