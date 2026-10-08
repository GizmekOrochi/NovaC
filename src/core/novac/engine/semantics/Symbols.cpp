#include "novac/engine/semantics/Symbols.hpp"

#include <functional>
#include <stdexcept>

namespace novac::semantics {

/**
 * @brief Creates a value through `create`.
 *
 * @param parent Value supplied for `parent`.
 * @param owner Value supplied for `owner`.
 * @return Value produced by the operation.
 */
ScopeId ScopeGraph::create(std::optional<ScopeId> parent, const ast::Node *owner) {
    if (parent.has_value() && !find(*parent)) {
        throw std::runtime_error("ScopeGraph::create: invalid parent scope");
    }
    const ScopeId id{static_cast<std::uint32_t>(scopes_.size())};
    scopes_.push_back(ScopeRecord{id, parent, owner, {}});
    if (owner) {
        nodeScopes_.insert_or_assign(owner, id);
    }
    return id;
}

/**
 * @brief Implements the `bind` operation.
 *
 * @param node Value supplied for `node`.
 * @param scope Value supplied for `scope`.
 */
void ScopeGraph::bind(const ast::Node &node, ScopeId scope) {
    static_cast<void>(require(scope));
    nodeScopes_.insert_or_assign(&node, scope);
}

/**
 * @brief Finds the value requested by `find`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
const ScopeRecord *ScopeGraph::find(ScopeId id) const noexcept {
    return id.value < scopes_.size() ? &scopes_[id.value] : nullptr;
}

/**
 * @brief Returns the value required by `require`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
const ScopeRecord &ScopeGraph::require(ScopeId id) const {
    const ScopeRecord *scope{find(id)};
    if (!scope) {
        throw std::runtime_error("ScopeGraph::require: invalid scope id");
    }
    return *scope;
}

/**
 * @brief Implements the `scopeOf` operation.
 *
 * @param node Value supplied for `node`.
 * @return Value produced by the operation.
 */
std::optional<ScopeId> ScopeGraph::scopeOf(const ast::Node &node) const noexcept {
    const auto it{nodeScopes_.find(&node)};
    return it == nodeScopes_.end() ? std::nullopt : std::optional<ScopeId>{it->second};
}

/**
 * @brief Implements the `parentOf` operation.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
std::optional<ScopeId> ScopeGraph::parentOf(ScopeId id) const {
    return require(id).parent;
}

/**
 * @brief Implements the `ancestors` operation.
 *
 * @param id Value supplied for `id`.
 * @param includeSelf Value supplied for `includeSelf`.
 * @return Value produced by the operation.
 */
std::vector<ScopeId> ScopeGraph::ancestors(ScopeId id, bool includeSelf) const {
    std::vector<ScopeId> result{};
    static_cast<void>(require(id));
    std::optional<ScopeId> current{includeSelf ? std::optional<ScopeId>{id} : parentOf(id)};
    while (current.has_value()) {
        result.push_back(*current);
        current = parentOf(*current);
    }
    return result;
}

/**
 * @brief Invokes the callable object.
 *
 * @param key Value supplied for `key`.
 * @return Value produced by the operation.
 */
std::size_t SymbolTable::ScopeNameHash::operator()(const ScopeNameKey &key) const noexcept {
    const std::size_t left{std::hash<std::uint32_t>{}(key.scope)};
    const std::size_t right{std::hash<std::string>{}(key.name)};
    return left ^ (right + 0x9e3779b9U + (left << 6U) + (left >> 2U));
}

/**
 * @brief Implements the `declare` operation.
 *
 * @param name Value supplied for `name`.
 * @param scope Value supplied for `scope`.
 * @param category Value supplied for `category`.
 * @param declaration Value supplied for `declaration`.
 * @return Value produced by the operation.
 */
SymbolId SymbolTable::declare(std::string name, ScopeId scope, std::string category, const ast::Node *declaration) {
    if (name.empty()) {
        throw std::runtime_error("SymbolTable::declare: symbol name cannot be empty");
    }
    static_cast<void>(scopes_->require(scope));
    const SymbolId id{static_cast<std::uint32_t>(symbols_.size())};
    symbols_.push_back(SymbolRecord{id, name, std::move(category), scope, declaration, {}});
    byScopeName_[ScopeNameKey{scope.value, std::move(name)}].push_back(id);
    return id;
}

/**
 * @brief Finds the value requested by `find`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
const SymbolRecord *SymbolTable::find(SymbolId id) const noexcept {
    return id.value < symbols_.size() ? &symbols_[id.value] : nullptr;
}

/**
 * @brief Returns the value required by `require`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
const SymbolRecord &SymbolTable::require(SymbolId id) const {
    const SymbolRecord *symbol{find(id)};
    if (!symbol) {
        throw std::runtime_error("SymbolTable::require: invalid symbol id");
    }
    return *symbol;
}

/**
 * @brief Implements the `mutableRecord` operation.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
SymbolRecord &SymbolTable::mutableRecord(SymbolId id) {
    if (id.value >= symbols_.size()) {
        throw std::runtime_error("SymbolTable::mutableRecord: invalid symbol id");
    }
    return symbols_[id.value];
}

/**
 * @brief Implements the `lookupLocal` operation.
 *
 * @param name Value supplied for `name`.
 * @param scope Value supplied for `scope`.
 * @return Value produced by the operation.
 */
std::vector<SymbolId> SymbolTable::lookupLocal(const std::string &name, ScopeId scope) const {
    static_cast<void>(scopes_->require(scope));
    const auto it{byScopeName_.find(ScopeNameKey{scope.value, name})};
    return it == byScopeName_.end() ? std::vector<SymbolId>{} : it->second;
}

/**
 * @brief Resolves data through `resolve`.
 *
 * @param name Value supplied for `name`.
 * @param from Value supplied for `from`.
 * @return Value produced by the operation.
 */
std::vector<SymbolId> SymbolTable::resolve(const std::string &name, ScopeId from) const {
    std::optional<ScopeId> current{from};
    while (current.has_value()) {
        std::vector<SymbolId> local{lookupLocal(name, *current)};
        if (!local.empty()) {
            return local;
        }
        current = scopes_->parentOf(*current);
    }
    return {};
}

/**
 * @brief Resolves data through `resolveFirst`.
 *
 * @param name Value supplied for `name`.
 * @param from Value supplied for `from`.
 * @return Value produced by the operation.
 */
std::optional<SymbolId> SymbolTable::resolveFirst(const std::string &name, ScopeId from) const {
    std::vector<SymbolId> matches{resolve(name, from)};
    return matches.empty() ? std::nullopt : std::optional<SymbolId>{matches.front()};
}

} // namespace novac::semantics
