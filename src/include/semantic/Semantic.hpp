#pragma once

#include "../ast/Node.hpp"
#include "../ids/Ids.hpp"
#include "../registry/Registry.hpp"

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace novac::semantic {

struct SymbolId {
    std::size_t value{};
};

struct ScopeId {
    std::size_t value{};
};

struct ReferenceId {
    std::size_t value{};
};

enum class SymbolKind {
    Unknown,
    Value,
    Type,
    Namespace,
    Module,
    Label
};

struct Symbol {
    SymbolId id{};
    std::string name{};
    SymbolKind kind{SymbolKind::Unknown};
    ast::NodePtr declaration{};
};

struct Reference {
    ReferenceId id{};
    std::string name{};
    ast::NodePtr node{};
    ScopeId scope{};
    std::optional<SymbolId> resolvedSymbol{};
};

class Scope {
public:
    Scope(ScopeId id, std::optional<ScopeId> parent);

    ScopeId id() const;
    std::optional<ScopeId> parent() const;

    bool define(std::string name, SymbolId symbol);
    std::optional<SymbolId> findLocal(const std::string &name) const;

private:
    ScopeId id_;
    std::optional<ScopeId> parent_;
    std::unordered_map<std::string, SymbolId> symbols_;
};

class SemanticContext {
public:
    SemanticContext();

    ScopeId rootScope() const;
    ScopeId currentScope() const;

    ScopeId pushScope();
    void popScope();

    SymbolId defineSymbol(std::string name, SymbolKind kind, ast::NodePtr declaration);
    ReferenceId addReference(std::string name, ast::NodePtr node);

    std::optional<SymbolId> resolve(const std::string &name) const;
    std::optional<SymbolId> resolveFrom(ScopeId scope, const std::string &name) const;

    const Symbol *symbol(SymbolId id) const;
    const Reference *reference(ReferenceId id) const;

    const std::vector<Symbol> &symbols() const;
    const std::vector<Reference> &references() const;
    const std::vector<Scope> &scopes() const;

private:
    Scope &scope(ScopeId id);
    const Scope &scope(ScopeId id) const;

    std::vector<Scope> scopes_;
    std::vector<Symbol> symbols_;
    std::vector<Reference> references_;
    std::vector<ScopeId> scopeStack_;
};

class SemanticRegistry {
public:
    using Handler = std::function<void(const ast::NodePtr &, SemanticContext &, const SemanticRegistry &)>;

    explicit SemanticRegistry(registry::DuplicatePolicy duplicatePolicy = registry::DuplicatePolicy::Error);

    registry::RegisterStatus node(std::string kind, Handler handler);
    registry::RegisterStatus node(const ids::NodeKind &kind, Handler handler);

    bool has(const std::string &kind) const;
    bool has(const ids::NodeKind &kind) const;

    void analyze(const ast::NodePtr &node, SemanticContext &context) const;
    void analyzeChildren(const ast::NodePtr &node, SemanticContext &context) const;

private:
    std::unordered_map<std::string, Handler> handlers_;
    registry::DuplicatePolicy duplicatePolicy_;
};

} // namespace novac::semantic