#pragma once

#include "novac/engine/foundation/Diagnostic.hpp"
#include "novac/engine/foundation/Metadata.hpp"
#include "novac/engine/foundation/registry/Registry.hpp"

#include <cstddef>
#include <cstdint>
#include <set>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace novac::compilation {

/** Type-safe, named storage for arbitrary compilation artifacts. */
class ArtifactStore final {
public:
    struct Descriptor {
        std::string kind;
        std::string target;
    };
    /** Native artifact metadata, if the producer supplied it. */
    const Descriptor *describe(const std::string &name) const noexcept {
        const auto it = descriptors_.find(name);
        return it == descriptors_.end() ? nullptr : &it->second;
    }
    /**
     * @brief Sets the value handled by `set`.
     *
     * @param name Value supplied for `name`.
     * @param value Value supplied for `value`.
     */
    template <typename T>
    void set(std::string name, T value) {
        if (name.empty()) {
            throw std::runtime_error("ArtifactStore::set: artifact name cannot be empty");
        }
        Entry entry{std::type_index(typeid(T)), std::make_shared<T>(std::move(value))};
        artifacts_.insert_or_assign(name, std::move(entry));
        descriptors_.erase(name);
    }

    /** All-or-nothing insertion of native byte artifacts. Existing artifact
     * entries remain unchanged if a name or allocation fails. */
    void insertByteBatch(std::vector<std::pair<std::string, std::vector<std::uint8_t>>> batch,
                         std::vector<Descriptor> descriptions = {}) {
        if (!descriptions.empty() && descriptions.size() != batch.size())
            throw std::runtime_error("ArtifactStore::insertByteBatch: descriptor count mismatch");
        std::set<std::string> names;
        auto candidate = artifacts_; // shared_ptr-owned values: cheap, no deep copies
        auto described = descriptors_;
        std::size_t index = 0;
        for (auto &item : batch) {
            if (item.first.empty() || !names.insert(item.first).second || candidate.contains(item.first))
                throw std::runtime_error("ArtifactStore::insertByteBatch: empty or duplicate artifact '" + item.first + "'");
            Entry entry{std::type_index(typeid(std::vector<std::uint8_t>)),
                        std::make_shared<std::vector<std::uint8_t>>(std::move(item.second))};
            if (!descriptions.empty()) described.emplace(item.first, std::move(descriptions[index]));
            candidate.emplace(std::move(item.first), std::move(entry));
            ++index;
        }
        artifacts_.swap(candidate); // noexcept commit
        descriptors_.swap(described);
    }

    /** Insert exactly once; a stage may not silently overwrite another stage's artifact. */
    template <typename T>
    void insert(std::string name, T value) {
        if (name.empty()) throw std::runtime_error("ArtifactStore::insert: empty name");
        if (has(name)) throw std::runtime_error("ArtifactStore::insert: duplicate artifact '" + name + "'");
        Entry entry{std::type_index(typeid(T)), std::make_shared<T>(std::move(value))};
        artifacts_.emplace(std::move(name), std::move(entry));
    }

    /**
     * @brief Performs the `emplace` operation.
     *
     * @param name Value supplied for `name`.
     * @param args Value supplied for `args`.
     * @return Value produced by the operation.
     */
    template <typename T, typename... Args>
    T &emplace(std::string name, Args &&...args) {
        if (name.empty()) {
            throw std::runtime_error("ArtifactStore::emplace: artifact name cannot be empty");
        }
        auto value{std::make_shared<T>(std::forward<Args>(args)...)};
        T &reference{*value};
        Entry entry{std::type_index(typeid(T)), std::move(value)};
        artifacts_.insert_or_assign(name, std::move(entry));
        descriptors_.erase(name);
        return reference;
    }

    /**
     * @brief Checks the condition represented by `has`.
     *
     * @param name Value supplied for `name`.
     * @return Value produced by the operation.
     */
    bool has(const std::string &name) const noexcept {
        return artifacts_.find(name) != artifacts_.end();
    }

    /**
     * @brief Returns the value exposed by `get`.
     *
     * @param name Value supplied for `name`.
     * @return Value produced by the operation.
     */
    template <typename T>
    T *get(const std::string &name) noexcept {
        const auto it{artifacts_.find(name)};
        if (it == artifacts_.end() || it->second.type != std::type_index(typeid(T))) {
            return nullptr;
        }
        return static_cast<T *>(it->second.value.get());
    }

    /**
     * @brief Returns the value exposed by `get`.
     *
     * @param name Value supplied for `name`.
     * @return Value produced by the operation.
     */
    template <typename T>
    const T *get(const std::string &name) const noexcept {
        const auto it{artifacts_.find(name)};
        if (it == artifacts_.end() || it->second.type != std::type_index(typeid(T))) {
            return nullptr;
        }
        return static_cast<const T *>(it->second.value.get());
    }

    /**
     * @brief Returns the value required by `require`.
     *
     * @param name Value supplied for `name`.
     * @return Value produced by the operation.
     */
    template <typename T>
    T &require(const std::string &name) {
        T *value{get<T>(name)};
        if (!value) {
            throw std::runtime_error("ArtifactStore::require: missing or mismatched artifact '" + name + "'");
        }
        return *value;
    }

    /**
     * @brief Returns the value required by `require`.
     *
     * @param name Value supplied for `name`.
     * @return Value produced by the operation.
     */
    template <typename T>
    const T &require(const std::string &name) const {
        const T *value{get<T>(name)};
        if (!value) {
            throw std::runtime_error("ArtifactStore::require: missing or mismatched artifact '" + name + "'");
        }
        return *value;
    }

    /**
     * @brief Performs the `erase` operation.
     *
     * @param name Value supplied for `name`.
     */
    void erase(const std::string &name) { artifacts_.erase(name); descriptors_.erase(name); }
    /**
     * @brief Resets state through `clear`.
     */
    void clear() noexcept { artifacts_.clear(); descriptors_.clear(); }
    /**
     * @brief Performs the `size` operation.
     *
     * @return Value produced by the operation.
     */
    std::size_t size() const noexcept { return artifacts_.size(); }

private:
    struct Entry {
        std::type_index type{typeid(void)};
        std::shared_ptr<void> value{};
    };

    std::unordered_map<std::string, Entry> artifacts_{};
    std::unordered_map<std::string, Descriptor> descriptors_{};
};

/** Per-compilation state shared by passes and backends. */
class CompilationSession final {
public:
    /**
     * @brief Performs the `artifacts` operation.
     *
     * @return Value produced by the operation.
     */
    ArtifactStore &artifacts() noexcept { return artifacts_; }
    /**
     * @brief Performs the `artifacts` operation.
     *
     * @return Value produced by the operation.
     */
    const ArtifactStore &artifacts() const noexcept { return artifacts_; }

    /**
     * @brief Performs the `diagnostics` operation.
     *
     * @return Value produced by the operation.
     */
    diagnostics::DiagnosticEngine &diagnostics() noexcept { return diagnostics_; }
    /**
     * @brief Performs the `diagnostics` operation.
     *
     * @return Value produced by the operation.
     */
    const diagnostics::DiagnosticEngine &diagnostics() const noexcept { return diagnostics_; }

    /**
     * @brief Returns the value exposed by `metadata`.
     *
     * @return Value produced by the operation.
     */
    metadata::MetadataStore &metadata() noexcept { return metadata_; }
    /**
     * @brief Returns the value exposed by `metadata`.
     *
     * @return Value produced by the operation.
     */
    const metadata::MetadataStore &metadata() const noexcept { return metadata_; }

private:
    ArtifactStore artifacts_{};
    diagnostics::DiagnosticEngine diagnostics_{};
    metadata::MetadataStore metadata_{};
};

struct PassContext {
    CompilationSession &session;
};

using PassFn = std::function<void(PassContext &)>;

struct PassDescriptor {
    std::string id{};
    std::string stage{};
    int priority{0};
    std::vector<std::string> after{};
    std::vector<std::string> before{};
    PassFn run{};
};

/**
 * Ordered registry of language-defined compiler passes.
 * Stage names are deliberately user-defined; NovaC does not prescribe a fixed
 * semantic or lowering pipeline.
 */
class PassRegistry final {
public:
    /**
     * @brief Constructs a `PassRegistry` instance.
     *
     * @param duplicatePolicy Value supplied for `duplicatePolicy`.
     */
    explicit PassRegistry(registry::DuplicatePolicy duplicatePolicy = registry::DuplicatePolicy::Error)
        : duplicatePolicy_{duplicatePolicy} {}

    /**
     * @brief Adds data through `add`.
     *
     * @param pass Value supplied for `pass`.
     * @return Value produced by the operation.
     */
    registry::RegisterStatus add(PassDescriptor pass);
    /**
     * @brief Checks the condition represented by `has`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    bool has(const std::string &id) const noexcept;
    /**
     * @brief Performs the `run` operation.
     *
     * @param stage Value supplied for `stage`.
     * @param session Value supplied for `session`.
     */
    void run(const std::string &stage, CompilationSession &session) const;
    /**
     * @brief Performs the `order` operation.
     *
     * @param stage Value supplied for `stage`.
     * @return Value produced by the operation.
     */
    std::vector<std::string> order(const std::string &stage) const;

private:
    struct Entry {
        PassDescriptor pass{};
        std::size_t registrationOrder{};
    };

    /**
     * @brief Performs the `orderedEntries` operation.
     *
     * @param stage Value supplied for `stage`.
     * @return Value produced by the operation.
     */
    std::vector<const Entry *> orderedEntries(const std::string &stage) const;

    std::vector<Entry> passes_{};
    std::unordered_map<std::string, std::size_t> indices_{};
    registry::DuplicatePolicy duplicatePolicy_;
};

} // namespace novac::compilation
