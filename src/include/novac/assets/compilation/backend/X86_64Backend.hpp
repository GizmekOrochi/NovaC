#pragma once
#include "novac/assets/compilation/backend/Backend.hpp"
#include "novac/assets/compilation/backend/x86_64/Codegen.hpp"

#include <utility>
#include <algorithm>

namespace novac::backend {

/** Experimental direct Linux x86-64 backend. No C, GCC, LLVM or assembler.
 * Generates byte-encoded machine code, ET_REL .o, and runnable ET_EXEC ELF64.
 * The baseline uses a private stack slot per SSA value and per block parameter.
 * Return type is i32; OS exit status is its low eight bits. */
class X86_64Backend final : public Backend {
public:
    /** Copy recipe packs at construction. No mutation during compilation. */
    explicit X86_64Backend(x86_64::ValueTypes types = {},
        compilation::machine::MachineRecipeRegistry custom = {},
        compilation::machine::MachineTarget target = {});
    std::optional<BackendCapabilities> capabilities() const override {
        BackendCapabilities result{"mir", "x86-64-elf-executable", {}, {"return", "jump", "branch"}};
        for (const auto &recipe : recipes_.recipes()) {
            if (recipe.target != target_.architecture ||
                !std::includes(target_.cpuFeatures.begin(),target_.cpuFeatures.end(),
                               recipe.requiredCpuFeatures.begin(),recipe.requiredCpuFeatures.end())) continue;
            if (std::find(result.instructions.begin(), result.instructions.end(), recipe.operation)==result.instructions.end())
                result.instructions.push_back(recipe.operation);
        }
        return result;
    }
    std::optional<ir::TypeRef> branchBooleanType() const override { return types_.boolean; }
    void preflight(const ir::MIRModule &mir) const override;
    void run(compilation::CompilationSession &session) const override;
private:
    x86_64::ValueTypes types_;
    compilation::machine::MachineRecipeRegistry recipes_;
    compilation::machine::MachineTarget target_;
};
} // namespace novac::backend
