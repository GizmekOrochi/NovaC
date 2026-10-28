#pragma once
#include "novac/assets/compilation/backend/Backend.hpp"
#include "novac/assets/compilation/CompilationOptions.hpp"
#include "novac/assets/compilation/target/TargetSpec.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace novac::backend {
/** Outputs of an architecture-specific encoder. The common runner owns
 * collision checks and publishes results only after the build completes. */
using NativeArtifactKind = BackendArtifactKind;
struct NativeArtifact {
    std::string name;
    std::vector<std::uint8_t> bytes;
    NativeArtifactKind kind{NativeArtifactKind::Opaque};
    /** Optional explicit metadata. If supplied it must match the provider. */
    std::optional<compilation::target::TargetSpec> target{};
};
struct NativeBuild {
    std::vector<NativeArtifact> artifacts;
};
/** Pluggable architecture and platform implementation. Neither Engine nor the
 * controller needs any knowledge of the instruction set or object format. */
class NativeTarget {
public:
    virtual ~NativeTarget() = default;
    virtual const compilation::target::TargetSpec& specification() const noexcept = 0;
    virtual BackendCapabilities capabilities() const = 0;
    virtual std::optional<ir::TypeRef> branchBooleanType() const { return std::nullopt; }
    virtual void preflight(const ir::MIRModule &mir) const = 0;
    virtual NativeBuild generate(const ir::MIRModule &mir) const = 0;
    /** Optional per-session variant. Existing derived targets keep working. */
    virtual NativeBuild generate(const ir::MIRModule &mir,
                                 const compilation::CompilationOptions &) const { return generate(mir); }
};
/** Shared native-backend lifecycle, independent of architecture. */
class NativeBackend : public Backend {
public:
    explicit NativeBackend(std::shared_ptr<const NativeTarget> target,
                           compilation::CompilationOptions options = {});
    std::optional<BackendCapabilities> capabilities() const override;
    std::optional<ir::TypeRef> branchBooleanType() const override;
    void preflight(const ir::MIRModule &mir) const override;
    void run(compilation::CompilationSession &session) const override;
    void runPrevalidated(compilation::CompilationSession &session) const override;
    const compilation::target::TargetSpec& specification() const noexcept;
private:
    void runImpl(compilation::CompilationSession &session, bool prevalidated) const;
    std::shared_ptr<const NativeTarget> target_;
    compilation::CompilationOptions options_{};
};
} // namespace novac::backend
