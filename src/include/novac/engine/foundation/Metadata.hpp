#pragma once

#include <any>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace novac::metadata {

/**
 * @brief Generic named metadata attached to compiler objects.
 *
 * MetadataStore deliberately carries no language semantics. Extensions can
 * attach arbitrary copyable C++ values under qualified string keys while core
 * NovaC types remain unaware of the meaning of those values.
 */
class MetadataStore final {
public:
    /**
     * @brief Sets the value handled by `set`.
     *
     * @param key Value supplied for `key`.
     * @param value Value supplied for `value`.
     * @return Value produced by the operation.
     */
    template <typename T>
    T &set(std::string key, T value) {
        ensureMutable();
        if (key.empty()) {
            throw std::runtime_error("MetadataStore::set: key cannot be empty");
        }
        std::any &slot{values_[std::move(key)]};
        slot = std::move(value);
        return std::any_cast<T &>(slot);
    }

    /**
     * @brief Performs the `emplace` operation.
     *
     * @param key Value supplied for `key`.
     * @param args Value supplied for `args`.
     * @return Value produced by the operation.
     */
    template <typename T, typename... Args>
    T &emplace(std::string key, Args &&...args) {
        return set<T>(std::move(key), T(std::forward<Args>(args)...));
    }

    /**
     * @brief Checks the condition represented by `has`.
     *
     * @param key Value supplied for `key`.
     * @return Value produced by the operation.
     */
    bool has(const std::string &key) const noexcept {
        return values_.find(key) != values_.end();
    }

    /**
     * @brief Returns the value exposed by `get`.
     *
     * @param key Value supplied for `key`.
     * @return Value produced by the operation.
     */
    template <typename T>
    T *get(const std::string &key) noexcept {
        const auto it{values_.find(key)};
        if (it == values_.end()) {
            return nullptr;
        }
        return std::any_cast<T>(&it->second);
    }

    /**
     * @brief Returns the value exposed by `get`.
     *
     * @param key Value supplied for `key`.
     * @return Value produced by the operation.
     */
    template <typename T>
    const T *get(const std::string &key) const noexcept {
        const auto it{values_.find(key)};
        if (it == values_.end()) {
            return nullptr;
        }
        return std::any_cast<T>(&it->second);
    }

    /**
     * @brief Returns the value required by `require`.
     *
     * @param key Value supplied for `key`.
     * @return Value produced by the operation.
     */
    template <typename T>
    T &require(const std::string &key) {
        T *value{get<T>(key)};
        if (!value) {
            throw std::runtime_error("MetadataStore::require: missing or mismatched metadata '" + key + "'");
        }
        return *value;
    }

    /**
     * @brief Returns the value required by `require`.
     *
     * @param key Value supplied for `key`.
     * @return Value produced by the operation.
     */
    template <typename T>
    const T &require(const std::string &key) const {
        const T *value{get<T>(key)};
        if (!value) {
            throw std::runtime_error("MetadataStore::require: missing or mismatched metadata '" + key + "'");
        }
        return *value;
    }

    /**
     * @brief Performs the `erase` operation.
     *
     * @param key Value supplied for `key`.
     */
    void erase(const std::string &key) {
        ensureMutable();
        values_.erase(key);
    }

    /**
     * @brief Resets state through `clear`.
     */
    void clear() {
        ensureMutable();
        values_.clear();
    }

    /**
     * @brief Performs the `keys` operation.
     *
     * @return Value produced by the operation.
     */
    std::vector<std::string> keys() const {
        std::vector<std::string> result{};
        result.reserve(values_.size());
        for (const auto &[key, value] : values_) {
            static_cast<void>(value);
            result.push_back(key);
        }
        return result;
    }

    /**
     * @brief Performs the `size` operation.
     *
     * @return Value produced by the operation.
     */
    std::size_t size() const noexcept { return values_.size(); }
    /**
     * @brief Checks the condition represented by `empty`.
     *
     * @return Value produced by the operation.
     */
    bool empty() const noexcept { return values_.empty(); }

    /**
     * @brief Performs the `freeze` operation.
     */
    void freeze() noexcept { frozen_ = true; }
    /**
     * @brief Performs the `frozen` operation.
     *
     * @return Value produced by the operation.
     */
    bool frozen() const noexcept { return frozen_; }

private:
    /**
     * @brief Ensures the invariant required by `ensureMutable`.
     */
    void ensureMutable() const {
        if (frozen_) {
            throw std::runtime_error("MetadataStore: metadata is frozen");
        }
    }

    std::unordered_map<std::string, std::any> values_{};
    bool frozen_{false};
};

} // namespace novac::metadata
