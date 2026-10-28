#include "novac/assets/compilation/backend/X86_64Backend.hpp"
#include "novac/assets/compilation/backend/x86_64/Elf64Writer.hpp"
#include "novac/assets/compilation/backend/x86_64/StandardRecipes.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <unordered_map>

namespace novac::backend {
LinuxX86_64Target::LinuxX86_64Target(x86_64::ValueTypes types,
        compilation::machine::MachineRecipeRegistry custom,
        compilation::machine::MachineTarget machineTarget)
        : types_{std::move(types)}, recipes_{x86_64::standardRecipes(types_)},
          machineTarget_{std::move(machineTarget)}, spec_{compilation::target::linuxX86_64()} {
    if (machineTarget_.architecture != "x86-64")
        throw std::runtime_error("X86_64Backend: unsupported target '" + machineTarget_.architecture + "'");
    spec_.cpuFeatures = machineTarget_.cpuFeatures;
    recipes_.append(custom);
    recipes_.freeze();
}
const compilation::target::TargetSpec& LinuxX86_64Target::specification() const noexcept { return spec_; }
BackendCapabilities LinuxX86_64Target::capabilities() const {
    BackendCapabilities result{"mir", "x86-64-elf-executable", {}, {"return", "jump", "branch"}};
    result.outputKind = NativeArtifactKind::Executable;
    for (const auto &recipe : recipes_.recipes()) {
        if (recipe.target != machineTarget_.architecture ||
            !std::includes(machineTarget_.cpuFeatures.begin(),machineTarget_.cpuFeatures.end(),
                recipe.requiredCpuFeatures.begin(),recipe.requiredCpuFeatures.end())) continue;
        if (std::find(result.instructions.begin(), result.instructions.end(),recipe.operation)==result.instructions.end())
            result.instructions.push_back(recipe.operation);
    }
    return result;
}
std::optional<ir::TypeRef> LinuxX86_64Target::branchBooleanType() const { return types_.boolean; }
void LinuxX86_64Target::preflight(const ir::MIRModule &mir) const {
    ir::validate(mir, {.requireTerminators=true, .requireResolvedTypes=true,
                       .branchBooleanType=types_.boolean});
    std::unordered_map<std::uint32_t,ir::TypeRef> definitions;
    const auto supported=[this](const ir::TypeRef &type) {
        return type==types_.i32 || type==types_.boolean || types_.layouts.has(type);
    };
    for (const auto &block : mir.blocks) {
        for (const auto &arg : block.arguments) {
            if (!supported(arg.type)) throw std::runtime_error(
                "X86_64Backend preflight: unsupported SSA type (no machine layout) for block argument '"+arg.type.display()+"'");
            definitions.emplace(arg.value.value,arg.type);
        }
        for (const auto &inst : block.instructions) {
            if (inst.result) {
                if (!supported(inst.type)) throw std::runtime_error(
                    "X86_64Backend preflight: unsupported SSA type (no machine layout) for result type '"+inst.type.display()+"'");
                definitions.emplace(inst.result->value,inst.type);
            }
        }
    }
    // Recipe selection runs exactly once in the per-compilation codegen plan.
    // Do not evaluate user-defined matches predicates here and then reselect
    // during emission; generation verifies all recipes before encoding bytes.
}
NativeBuild LinuxX86_64Target::generate(const ir::MIRModule &mir) const {
    return generateForTarget(mir, machineTarget_);
}
NativeBuild LinuxX86_64Target::generate(const ir::MIRModule &mir,
        const compilation::CompilationOptions &options) const {
    // The canonical virtual hook must receive exactly the target for this
    // session. Dispatching through generate(mir) for derived classes loses the
    // requested optimization goal (and other future session settings).
    auto configured = machineTarget_;
    if (options.optimization) configured.goal = *options.optimization;
    return generateForTarget(mir, configured);
}
NativeBuild LinuxX86_64Target::generateForTarget(const ir::MIRModule &mir,
        const compilation::machine::MachineTarget &target) const {
    const auto image = x86_64::selectAndEncode(mir, types_, recipes_, target);
    auto object = x86_64::makeObject(image.text, image.mainOffset);
    auto executable = x86_64::makeExecutable(image.text);
    return {{{"x86-64-machine-code", image.text, NativeArtifactKind::MachineCode, spec_},
             {"x86-64-elf-object",std::move(object), NativeArtifactKind::ObjectFile, spec_},
             {"x86-64-elf-executable",std::move(executable), NativeArtifactKind::Executable, spec_}}};
}
std::shared_ptr<const NativeTarget> makeX86_64LinuxTarget(x86_64::ValueTypes types,
        compilation::machine::MachineRecipeRegistry custom,
        compilation::machine::MachineTarget target) {
    return std::make_shared<LinuxX86_64Target>(std::move(types),std::move(custom),std::move(target));
}
X86_64Backend::X86_64Backend(x86_64::ValueTypes types,
        compilation::machine::MachineRecipeRegistry custom,
        compilation::machine::MachineTarget target)
    : NativeBackend{makeX86_64LinuxTarget(std::move(types),std::move(custom),std::move(target))} {}
} // namespace novac::backend
