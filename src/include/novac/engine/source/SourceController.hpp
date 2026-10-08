#pragma once

#include "Source.hpp"
#include "../foundation/registry/Registry.hpp"

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace novac::source {

/** @brief Callback capable of resolving one logical source request. */
using SourceResolver = std::function<std::optional<Source>(const SourceRequest &)>;

/**
 * @brief Resolves source units through an ordered, extensible resolver chain.
 *
 * NovaC deliberately does not hard-code filesystem access. Applications may
 * resolve source from files, packages, archives, IDE buffers, memory, or any
 * other storage by registering callbacks.
 */
class SourceController {
public:
    /**
     * @brief Constructs a `SourceController` instance.
     *
     * @param duplicatePolicy Value supplied for `duplicatePolicy`.
     */
    explicit SourceController(
        registry::DuplicatePolicy duplicatePolicy = registry::DuplicatePolicy::Error);

    /**
     * @brief Adds one resolver to the ordered source-resolution chain.
     *
     * Resolvers are queried in registration order until one returns a Source.
     * Duplicate ids follow the controller's DuplicatePolicy.
     *
     * @param id Registration id used for duplicate handling and diagnostics.
     * @param resolver Callback to invoke for unresolved source requests.
     * @return Registration result.
     * @throws std::runtime_error If id or resolver is empty, or a duplicate is rejected.
     */
    registry::RegisterStatus resolver(std::string id, SourceResolver resolver);

    /**
     * @brief Resolves one request using registered resolvers in order.
     *
     * If a resolver returns a Source with an empty name, its canonical id is
     * copied into name before the Source is returned.
     *
     * @param request Logical source request.
     * @return First successfully resolved source.
     * @throws std::runtime_error If the specifier is empty, no resolver accepts
     * the request, or a resolver returns a source without a canonical id.
     */
    Source resolve(const SourceRequest &request) const;

private:
    struct ResolverEntry {
        std::string id{};
        SourceResolver resolver{};
    };

    std::vector<ResolverEntry> resolvers_{};
    std::unordered_map<std::string, std::size_t> resolverIndices_{};
    registry::DuplicatePolicy duplicatePolicy_;
};

} // namespace novac::source
