#pragma once

#include "novac/engine/foundation/Metadata.hpp"
#include "novac/engine/foundation/registry/Registry.hpp"
#include "novac/engine/syntax/Node.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace novac::semantics {

struct ScopeId {
    std::uint32_t value{};
    friend bool operator==(const ScopeId &, const ScopeId &) = default;
};

struct SymbolId {
    std::uint32_t value{};
    friend bool operator==(const SymbolId &, const SymbolId &) = default;
};

struct ScopeRecord {
    ScopeId id{};
    std::optional<ScopeId> parent{};
    const ast::Node *owner{};
    metadata::MetadataStore metadata{};
};

/** Generic lexical-scope graph, independent from runtime environments. */
class ScopeGraph final {
public:
    /**
     * @brief Creates a value through `create`.
     *
     * @param parent Value supplied for `parent`.
     * @param owner Value supplied for `owner`.
     * @return Value produced by the operation.
     */
    ScopeId create(std::optional<ScopeId> parent = std::nullopt, const ast::Node *owner = nullptr);
    /**
     * @brief Performs the `bind` operation.
     *
     * @param node Value supplied for `node`.
     * @param scope Value supplied for `scope`.
     */
    void bind(const ast::Node &node, ScopeId scope);

    /**
     * @brief Finds the value requested by `find`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    const ScopeRecord *find(ScopeId id) const noexcept;
    /**
     * @brief Returns the value required by `require`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    const ScopeRecord &require(ScopeId id) const;
    /**
     * @brief Performs the `scopeOf` operation.
     *
     * @param node Value supplied for `node`.
     * @return Value produced by the operation.
     */
    std::optional<ScopeId> scopeOf(const ast::Node &node) const noexcept;
    /**
     * @brief Performs the `parentOf` operation.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    std::optional<ScopeId> parentOf(ScopeId id) const;
    /**
     * @brief Performs the `ancestors` operation.
     *
     * @param id Value supplied for `id`.
     * @param includeSelf Value supplied for `includeSelf`.
     * @return Value produced by the operation.
     */
    std::vector<ScopeId> ancestors(ScopeId id, bool includeSelf = false) const;

private:
    std::vector<ScopeRecord> scopes_{};
    std::unordered_map<const ast::Node *, ScopeId> nodeScopes_{};
};

struct SymbolRecord {
    SymbolId id{};
    std::string name{};
    std::string category{};
    ScopeId scope{};
    const ast::Node *declaration{};
    metadata::MetadataStore metadata{};
};

/**
 * Generic symbol index layered over ScopeGraph. It provides lexical lookup but
 * does not prescribe declaration kinds, visibility, overload rules, or types.
 */
class SymbolTable final {
public:
    /**
     * @brief Constructs a `SymbolTable` instance.
     *
     * @param scopes Value supplied for `scopes`.
     */
    explicit SymbolTable(const ScopeGraph &scopes) : scopes_{&scopes} {}

    /**
     * @brief Performs the `declare` operation.
     *
     * @param name Value supplied for `name`.
     * @param scope Value supplied for `scope`.
     * @param category Value supplied for `category`.
     * @param declaration Value supplied for `declaration`.
     * @return Value produced by the operation.
     */
    SymbolId declare(std::string name, ScopeId scope, std::string category = {}, const ast::Node *declaration = nullptr);
    /**
     * @brief Finds the value requested by `find`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    const SymbolRecord *find(SymbolId id) const noexcept;
    /**
     * @brief Returns the value required by `require`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    const SymbolRecord &require(SymbolId id) const;

    /**
     * @brief Performs the `lookupLocal` operation.
     *
     * @param name Value supplied for `name`.
     * @param scope Value supplied for `scope`.
     * @return Value produced by the operation.
     */
    std::vector<SymbolId> lookupLocal(const std::string &name, ScopeId scope) const;
    /**
     * @brief Resolves data through `resolve`.
     *
     * @param name Value supplied for `name`.
     * @param from Value supplied for `from`.
     * @return Value produced by the operation.
     */
    std::vector<SymbolId> resolve(const std::string &name, ScopeId from) const;
    /**
     * @brief Resolves data through `resolveFirst`.
     *
     * @param name Value supplied for `name`.
     * @param from Value supplied for `from`.
     * @return Value produced by the operation.
     */
    std::optional<SymbolId> resolveFirst(const std::string &name, ScopeId from) const;

    /**
     * @brief Performs the `mutableRecord` operation.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    SymbolRecord &mutableRecord(SymbolId id);

private:
    struct ScopeNameKey {
        std::uint32_t scope{};
        std::string name{};
        friend bool operator==(const ScopeNameKey &, const ScopeNameKey &) = default;
    };

    struct ScopeNameHash {
        /**
         * @brief Invokes the callable object.
         *
         * @param key Value supplied for `key`.
         * @return Value produced by the operation.
         */
        std::size_t operator()(const ScopeNameKey &key) const noexcept;
    };

    const ScopeGraph *scopes_{};
    std::vector<SymbolRecord> symbols_{};
    std::unordered_map<ScopeNameKey, std::vector<SymbolId>, ScopeNameHash> byScopeName_{};
};

} // namespace novac::semantics
