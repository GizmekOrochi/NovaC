#pragma once

#include "novac/engine/compilation/Compilation.hpp"
#include "novac/engine/foundation/registry/Registry.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace novac::backend {

/** Backend contract with no prescribed output format or execution model. */
class Backend {
public:
    /**
     * @brief Destroys the `Backend` instance.
     */
    virtual ~Backend() = default;
    /**
     * @brief Performs the `run` operation.
     *
     * @param session Value supplied for `session`.
     */
    virtual void run(compilation::CompilationSession &session) const = 0;
};

class BackendRegistry final {
public:
    /**
     * @brief Constructs a `BackendRegistry` instance.
     *
     * @param duplicatePolicy Value supplied for `duplicatePolicy`.
     */
    explicit BackendRegistry(registry::DuplicatePolicy duplicatePolicy = registry::DuplicatePolicy::Error)
        : duplicatePolicy_{duplicatePolicy} {}

    /**
     * @brief Adds data through `add`.
     *
     * @param id Value supplied for `id`.
     * @param backend Value supplied for `backend`.
     * @return Value produced by the operation.
     */
    registry::RegisterStatus add(std::string id, std::shared_ptr<const Backend> backend);
    /**
     * @brief Checks the condition represented by `has`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    bool has(const std::string &id) const noexcept;
    /**
     * @brief Returns the value required by `require`.
     *
     * @param id Value supplied for `id`.
     * @return Value produced by the operation.
     */
    const Backend &require(const std::string &id) const;
    /**
     * @brief Performs the `run` operation.
     *
     * @param id Value supplied for `id`.
     * @param session Value supplied for `session`.
     */
    void run(const std::string &id, compilation::CompilationSession &session) const;

private:
    std::unordered_map<std::string, std::shared_ptr<const Backend>> backends_{};
    registry::DuplicatePolicy duplicatePolicy_;
};

} // namespace novac::backend
