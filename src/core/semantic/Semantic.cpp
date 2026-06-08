#include "../../include/semantic/Semantic.hpp"

#include "../../include/registry/RegistryHelpers.hpp"

#include <stdexcept>
#include <utility>
#include <variant>

namespace novac::semantic {

Scope::Scope(ScopeId id, std::optional<ScopeId> parent)
    : id_{id}, parent_{parent}, symbols_{} {}

ScopeId Scope::id() const {
    return id_;
}

std::optional<ScopeId> Scope::parent() const {
    return parent_;
}

bool Scope::define(std::string name, SymbolId symbol) {
    return symbols_.emplace(std::move(name), symbol).second;
}

std::optional<SymbolId> Scope::findLocal(const std::string &name) const {
    const auto iter{symbols_.find(name)};

    if (iter == symbols_.end())
        return std::nullopt;

    return iter->second;
}

SemanticContext::SemanticContext()
    : scopes_{}, symbols_{}, references_{}, scopeStack_{} {
    scopes_.push_back(Scope{ScopeId{0}, std::nullopt});
    scopeStack_.push_back(ScopeId{0});
}

ScopeId SemanticContext::rootScope() const {
    return ScopeId{0};
}

ScopeId SemanticContext::currentScope() const {
    if (scopeStack_.empty())
        throw std::runtime_error("SemanticContext::currentScope: no active scope");

    return scopeStack_.back();
}

ScopeId SemanticContext::pushScope() {
    const ScopeId parent{currentScope()};
    const ScopeId id{scopes_.size()};

    scopes_.push_back(Scope{id, parent});
    scopeStack_.push_back(id);

    return id;
}

void SemanticContext::popScope() {
    if (scopeStack_.size() <= 1)
        throw std::runtime_error("SemanticContext::popScope: cannot pop root scope");

    scopeStack_.pop_back();
}

SymbolId SemanticContext::defineSymbol( std::string name, SymbolKind kind, ast::NodePtr declaration) {
    const SymbolId id{symbols_.size()};

    Symbol symbol{id, name, kind, std::move(declaration)};

    if (!scope(currentScope()).define(name, id))
        throw std::runtime_error("SemanticContext::defineSymbol: duplicate symbol '" + name + "'");

    symbols_.push_back(std::move(symbol));

    return id;
}

ReferenceId SemanticContext::addReference(std::string name, ast::NodePtr node) {
    const ReferenceId id{references_.size()};
    const std::optional<SymbolId> resolved{resolve(name)};

    references_.push_back({id, std::move(name), std::move(node), currentScope(), resolved});

    return id;
}

std::optional<SymbolId> SemanticContext::resolve(const std::string &name) const {
    return resolveFrom(currentScope(), name);
}

std::optional<SymbolId> SemanticContext::resolveFrom(ScopeId scopeId, const std::string &name) const {
    const Scope *current{&scope(scopeId)};

    while (current) {
        if (std::optional<SymbolId> symbol{current->findLocal(name)})
            return symbol;

        if (!current->parent())
            break;

        current = &scope(*current->parent());
    }

    return std::nullopt;
}

const Symbol *SemanticContext::symbol(SymbolId id) const {
    if (id.value >= symbols_.size()) {
        return nullptr;
    }

    return &symbols_[id.value];
}

const Reference *SemanticContext::reference(ReferenceId id) const {
    if (id.value >= references_.size()) {
        return nullptr;
    }

    return &references_[id.value];
}

const std::vector<Symbol> &SemanticContext::symbols() const {
    return symbols_;
}

const std::vector<Reference> &SemanticContext::references() const {
    return references_;
}

const std::vector<Scope> &SemanticContext::scopes() const {
    return scopes_;
}

Scope &SemanticContext::scope(ScopeId id) {
    if (id.value >= scopes_.size())
        throw std::runtime_error("SemanticContext::scope: invalid scope id");

    return scopes_[id.value];
}

const Scope &SemanticContext::scope(ScopeId id) const {
    if (id.value >= scopes_.size())
        throw std::runtime_error("SemanticContext::scope: invalid scope id");

    return scopes_[id.value];
}

SemanticRegistry::SemanticRegistry(registry::DuplicatePolicy duplicatePolicy)
    : handlers_{}, duplicatePolicy_{duplicatePolicy} {}

registry::RegisterStatus SemanticRegistry::node(std::string kind, Handler handler) {
    return registry::registerEntry(handlers_, std::move(kind), std::move(handler), duplicatePolicy_, "SemanticRegistry::node");
}

registry::RegisterStatus SemanticRegistry::node(const ids::NodeKind &kind, Handler handler) {
    return node(kind.value, std::move(handler));
}

bool SemanticRegistry::has(const std::string &kind) const {
    return handlers_.find(kind) != handlers_.end();
}

bool SemanticRegistry::has(const ids::NodeKind &kind) const {
    return has(kind.value);
}

void SemanticRegistry::analyze(const ast::NodePtr &node, SemanticContext &context) const {
    if (!node)
        return;

    const auto iter{handlers_.find(node->kind())};

    if (iter != handlers_.end()) {
        iter->second(node, context, *this);
        return;
    }

    analyzeChildren(node, context);
}

void SemanticRegistry::analyzeChildren(const ast::NodePtr &node, SemanticContext &context) const {
    if (!node)
        return;

    for (const auto &[name, field] : node->fields()) {
        static_cast<void>(name);

        if (const auto *child{std::get_if<ast::NodePtr>(&field)})
            analyze(*child, context);

        if (const auto *list{std::get_if<ast::NodeList>(&field)})
            for (const ast::NodePtr &child : *list)
                analyze(child, context);
    }
}

} // namespace novac::semantic