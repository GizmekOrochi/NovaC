#include "novac/engine/compilation/Compilation.hpp"

#include <algorithm>
#include <queue>
#include <unordered_set>

namespace novac::compilation {

/**
 * @brief Adds data through `add`.
 *
 * @param pass Value supplied for `pass`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus PassRegistry::add(PassDescriptor pass) {
    if (pass.id.empty()) {
        throw std::runtime_error("PassRegistry::add: pass id cannot be empty");
    }
    if (pass.stage.empty()) {
        throw std::runtime_error("PassRegistry::add: stage cannot be empty");
    }
    if (!pass.run) {
        throw std::runtime_error("PassRegistry::add: pass callback cannot be empty");
    }

    const auto existing{indices_.find(pass.id)};
    if (existing != indices_.end()) {
        if (duplicatePolicy_ == registry::DuplicatePolicy::Ignore) {
            return registry::RegisterStatus::Ignored;
        }
        if (duplicatePolicy_ == registry::DuplicatePolicy::Error) {
            throw std::runtime_error("PassRegistry::add: duplicate pass '" + pass.id + "'");
        }
        passes_[existing->second].pass = std::move(pass);
        return registry::RegisterStatus::Replaced;
    }

    const std::size_t index{passes_.size()};
    indices_.emplace(pass.id, index);
    passes_.push_back(Entry{std::move(pass), index});
    return registry::RegisterStatus::Inserted;
}

/**
 * @brief Checks the condition represented by `has`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
bool PassRegistry::has(const std::string &id) const noexcept {
    return indices_.find(id) != indices_.end();
}

/**
 * @brief Implements the `orderedEntries` operation.
 *
 * @param stage Value supplied for `stage`.
 * @return Value produced by the operation.
 */
std::vector<const PassRegistry::Entry *> PassRegistry::orderedEntries(const std::string &stage) const {
    std::vector<const Entry *> nodes{};
    for (const Entry &entry : passes_) {
        if (entry.pass.stage == stage) {
            nodes.push_back(&entry);
        }
    }

    std::unordered_map<std::string, std::size_t> localIndex{};
    for (std::size_t i{}; i < nodes.size(); ++i) {
        localIndex.emplace(nodes[i]->pass.id, i);
    }

    std::vector<std::vector<std::size_t>> edges(nodes.size());
    std::vector<std::size_t> indegree(nodes.size(), 0);
    std::unordered_set<std::string> edgeKeys{};

    auto addEdge = [&](std::size_t from, std::size_t to) {
        const std::string key{std::to_string(from) + ":" + std::to_string(to)};
        if (!edgeKeys.insert(key).second) {
            return;
        }
        edges[from].push_back(to);
        ++indegree[to];
    };

    for (std::size_t i{}; i < nodes.size(); ++i) {
        for (const std::string &dependency : nodes[i]->pass.after) {
            const auto it{localIndex.find(dependency)};
            if (it == localIndex.end()) {
                throw std::runtime_error("PassRegistry::order: pass '" + nodes[i]->pass.id + "' depends on missing pass '" + dependency + "' in stage '" + stage + "'");
            }
            addEdge(it->second, i);
        }
        for (const std::string &dependency : nodes[i]->pass.before) {
            const auto it{localIndex.find(dependency)};
            if (it == localIndex.end()) {
                throw std::runtime_error("PassRegistry::order: pass '" + nodes[i]->pass.id + "' references missing pass '" + dependency + "' in stage '" + stage + "'");
            }
            addEdge(i, it->second);
        }
    }

    auto lessPreferred = [&](std::size_t left, std::size_t right) {
        const Entry *a{nodes[left]};
        const Entry *b{nodes[right]};
        if (a->pass.priority != b->pass.priority) {
            return a->pass.priority < b->pass.priority;
        }
        return a->registrationOrder > b->registrationOrder;
    };

    std::priority_queue<std::size_t, std::vector<std::size_t>, decltype(lessPreferred)> ready{lessPreferred};
    for (std::size_t i{}; i < nodes.size(); ++i) {
        if (indegree[i] == 0) {
            ready.push(i);
        }
    }

    std::vector<const Entry *> result{};
    result.reserve(nodes.size());
    while (!ready.empty()) {
        const std::size_t index{ready.top()};
        ready.pop();
        result.push_back(nodes[index]);
        for (const std::size_t target : edges[index]) {
            if (--indegree[target] == 0) {
                ready.push(target);
            }
        }
    }

    if (result.size() != nodes.size()) {
        throw std::runtime_error("PassRegistry::order: dependency cycle in stage '" + stage + "'");
    }
    return result;
}

/**
 * @brief Implements the `order` operation.
 *
 * @param stage Value supplied for `stage`.
 * @return Value produced by the operation.
 */
std::vector<std::string> PassRegistry::order(const std::string &stage) const {
    std::vector<std::string> result{};
    for (const Entry *entry : orderedEntries(stage)) {
        result.push_back(entry->pass.id);
    }
    return result;
}

/**
 * @brief Implements the `run` operation.
 *
 * @param stage Value supplied for `stage`.
 * @param session Value supplied for `session`.
 */
void PassRegistry::run(const std::string &stage, CompilationSession &session) const {
    PassContext context{session};
    for (const Entry *entry : orderedEntries(stage)) {
        entry->pass.run(context);
    }
}

} // namespace novac::compilation
