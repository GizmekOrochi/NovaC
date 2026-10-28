#pragma once

#include "novac/assets/compilation/Compilation.hpp"
#include "novac/engine/foundation/registry/Registry.hpp"
#include "novac/engine/transformation/TypeIdentity.hpp"

#include <memory>
#include <optional>
#include <vector>
#include <string>
#include <unordered_map>

namespace novac::ir { struct MIRModule; }
namespace novac::backend {

/** Auditable backend contract for the modular path. The old run-only backend
 * remains usable by the classic facade, but cannot be run through runChecked. */
struct BackendCapabilities {
    std::string inputArtifact{"mir"};
    std::string outputArtifact{};
    std::vector<std::string> instructions{};
    std::vector<std::string> terminators{};
};

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
    /** Explicit operation coverage for preflight; no implicit fallback. */
    virtual std::optional<BackendCapabilities> capabilities() const { return std::nullopt; }
    /** Optional target-aware MIR preflight. Default for legacy backends. */
    virtual void preflight(const ir::MIRModule &) const {}
    /** Custom logical boolean identity for standard branch semantics. */
    virtual std::optional<ir::TypeRef> branchBooleanType() const { return std::nullopt; }
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
    /** Preflight all MIR operations before executing an opt-in modular backend. */
    void runChecked(const std::string &id, compilation::CompilationSession &session) const;

private:
    std::unordered_map<std::string, std::shared_ptr<const Backend>> backends_{};
    registry::DuplicatePolicy duplicatePolicy_;
};

} // namespace novac::backend
