#pragma once
#include "novac/assets/compilation/backend/NativeBackend.hpp"
#include "novac/assets/compilation/backend/x86_64/Codegen.hpp"
#include "novac/assets/compilation/machine/MachineRecipe.hpp"

namespace novac::backend {
/** Linux x86-64 target provider. Subclasses customize generateForTarget(),
 * the canonical hook invoked for ALL session configurations. Overrides of the
 * legacy generate(mir) alone do not customize session-aware compilation;
 * migrate these overrides to generateForTarget() to receive the correct goal.
 * The inherited interface is independent of CompilationController. */
class LinuxX86_64Target : public NativeTarget {
public:
    explicit LinuxX86_64Target(x86_64::ValueTypes types = {},
        compilation::machine::MachineRecipeRegistry custom = {},
        compilation::machine::MachineTarget target = {});
    const compilation::target::TargetSpec& specification() const noexcept override;
    BackendCapabilities capabilities() const override;
    std::optional<ir::TypeRef> branchBooleanType() const override;
    void preflight(const ir::MIRModule &mir) const override;
    NativeBuild generate(const ir::MIRModule &mir) const override;
    NativeBuild generate(const ir::MIRModule &mir, const compilation::CompilationOptions &options) const override;
protected:
    /** Extension point for downstream native targets, called through the
     * options-aware public API with the effective goal for this session. */
    virtual NativeBuild generateForTarget(const ir::MIRModule &mir,
        const compilation::machine::MachineTarget &target) const;
    const x86_64::ValueTypes& valueTypes() const noexcept { return types_; }
    const compilation::machine::MachineRecipeRegistry& recipes() const noexcept { return recipes_; }
    const compilation::machine::MachineTarget& machineTarget() const noexcept { return machineTarget_; }
private:
    x86_64::ValueTypes types_;
    compilation::machine::MachineRecipeRegistry recipes_;
    compilation::machine::MachineTarget machineTarget_;
    compilation::target::TargetSpec spec_;
};

/** Compatibility facade for the first supported native target. The shared
 * lifecycle and output checks live in NativeBackend, not this class. */
class X86_64Backend : public NativeBackend {
public:
    explicit X86_64Backend(x86_64::ValueTypes types = {},
        compilation::machine::MachineRecipeRegistry custom = {},
        compilation::machine::MachineTarget target = {});
};
/** Plugin-style constructor, usable by TargetRegistry without requiring the
 * general CompilationController or Engine to know the x86-64 architecture. */
std::shared_ptr<const NativeTarget> makeX86_64LinuxTarget(
    x86_64::ValueTypes types = {},
    compilation::machine::MachineRecipeRegistry custom = {},
    compilation::machine::MachineTarget target = {});
} // namespace novac::backend
