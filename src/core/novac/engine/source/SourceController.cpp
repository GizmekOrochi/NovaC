#include "novac/engine/source/SourceController.hpp"

#include <stdexcept>
#include <utility>

namespace novac::source {

/**
 * @brief Constructs a `SourceController` instance.
 *
 * @param duplicatePolicy Value supplied for `duplicatePolicy`.
 */
SourceController::SourceController(registry::DuplicatePolicy duplicatePolicy)
    : resolvers_{}, resolverIndices_{}, duplicatePolicy_{duplicatePolicy} {}

/**
 * @brief Resolves data through `resolver`.
 *
 * @param id Value supplied for `id`.
 * @param resolverValue Value supplied for `resolverValue`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus SourceController::resolver(std::string id, SourceResolver resolverValue) {
    if (id.empty()) {
        throw std::runtime_error("SourceController::resolver: id cannot be empty");
    }
    if (!resolverValue) {
        throw std::runtime_error("SourceController::resolver: resolver cannot be empty");
    }

    const auto iter{resolverIndices_.find(id)};
    if (iter != resolverIndices_.end()) {
        if (duplicatePolicy_ == registry::DuplicatePolicy::Ignore) {
            return registry::RegisterStatus::Ignored;
        }
        if (duplicatePolicy_ == registry::DuplicatePolicy::Replace) {
            resolvers_[iter->second].resolver = std::move(resolverValue);
            return registry::RegisterStatus::Replaced;
        }
        throw std::runtime_error("SourceController::resolver: duplicate registration '" + id + "'");
    }

    const std::size_t index{resolvers_.size()};
    resolvers_.push_back({id, std::move(resolverValue)});
    resolverIndices_.emplace(std::move(id), index);
    return registry::RegisterStatus::Inserted;
}

/**
 * @brief Resolves data through `resolve`.
 *
 * @param request Value supplied for `request`.
 * @return Value produced by the operation.
 */
Source SourceController::resolve(const SourceRequest &request) const {
    if (request.specifier.empty()) {
        throw std::runtime_error("SourceController::resolve: source specifier cannot be empty");
    }

    for (const ResolverEntry &entry : resolvers_) {
        std::optional<Source> resolved{entry.resolver(request)};
        if (!resolved) {
            continue;
        }
        if (resolved->id.empty()) {
            throw std::runtime_error(
                "SourceController::resolve: resolver '" + entry.id + "' returned a source without an id");
        }
        if (resolved->name.empty()) {
            resolved->name = resolved->id;
        }
        return std::move(*resolved);
    }

    throw std::runtime_error(
        "SourceController::resolve: unable to resolve source '" + request.specifier + "'");
}

} // namespace novac::source
