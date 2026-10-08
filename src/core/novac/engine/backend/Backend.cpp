#include "novac/engine/backend/Backend.hpp"

#include <stdexcept>
#include <utility>

namespace novac::backend {

/**
 * @brief Adds data through `add`.
 *
 * @param id Value supplied for `id`.
 * @param backend Value supplied for `backend`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus BackendRegistry::add(std::string id, std::shared_ptr<const Backend> backend) {
    if (id.empty()) {
        throw std::runtime_error("BackendRegistry::add: backend id cannot be empty");
    }
    if (!backend) {
        throw std::runtime_error("BackendRegistry::add: backend cannot be null");
    }

    const auto it{backends_.find(id)};
    if (it != backends_.end()) {
        if (duplicatePolicy_ == registry::DuplicatePolicy::Ignore) {
            return registry::RegisterStatus::Ignored;
        }
        if (duplicatePolicy_ == registry::DuplicatePolicy::Error) {
            throw std::runtime_error("BackendRegistry::add: duplicate backend '" + id + "'");
        }
        it->second = std::move(backend);
        return registry::RegisterStatus::Replaced;
    }

    backends_.emplace(std::move(id), std::move(backend));
    return registry::RegisterStatus::Inserted;
}

/**
 * @brief Checks the condition represented by `has`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
bool BackendRegistry::has(const std::string &id) const noexcept {
    return backends_.find(id) != backends_.end();
}

/**
 * @brief Returns the value required by `require`.
 *
 * @param id Value supplied for `id`.
 * @return Value produced by the operation.
 */
const Backend &BackendRegistry::require(const std::string &id) const {
    const auto it{backends_.find(id)};
    if (it == backends_.end()) {
        throw std::runtime_error("BackendRegistry::require: unknown backend '" + id + "'");
    }
    return *it->second;
}

/**
 * @brief Implements the `run` operation.
 *
 * @param id Value supplied for `id`.
 * @param session Value supplied for `session`.
 */
void BackendRegistry::run(const std::string &id, compilation::CompilationSession &session) const {
    require(id).run(session);
}

} // namespace novac::backend
