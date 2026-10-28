#include "novac/assets/compilation/backend/NativeBackend.hpp"
#include "novac/engine/transformation/IR.hpp"
#include "novac/assets/compilation/object/Elf64Verifier.hpp"
#include <set>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace novac::backend {
NativeBackend::NativeBackend(std::shared_ptr<const NativeTarget> target,
                             compilation::CompilationOptions options)
    : target_{std::move(target)}, options_{std::move(options)} {
    if (!target_) throw std::runtime_error("NativeBackend: null target");
    target_->specification().validate();
}
std::optional<BackendCapabilities> NativeBackend::capabilities() const { return target_->capabilities(); }
std::optional<ir::TypeRef> NativeBackend::branchBooleanType() const { return target_->branchBooleanType(); }
void NativeBackend::preflight(const ir::MIRModule& mir) const {
    ir::validate(mir, {.requireTerminators = true,
                       .branchBooleanType = target_->branchBooleanType()});
    target_->preflight(mir);
}
const compilation::target::TargetSpec& NativeBackend::specification() const noexcept {
    return target_->specification();
}
void NativeBackend::run(compilation::CompilationSession &session) const { runImpl(session, false); }
void NativeBackend::runPrevalidated(compilation::CompilationSession &session) const { runImpl(session, true); }
void NativeBackend::runImpl(compilation::CompilationSession &session, bool prevalidated) const {
    const auto cap = target_->capabilities();
    if (cap.inputArtifact.empty() || cap.outputArtifact.empty())
        throw std::runtime_error("NativeBackend: invalid artifact contract");
    const auto &mir = session.artifacts().require<ir::MIRModule>(cap.inputArtifact);
    // The public run() and runPrevalidated() entry points MUST always enforce
    // structural IR safety. Provider preflight is optional, not a substitute.
    ir::validate(mir, {.requireTerminators = true,
                       .branchBooleanType = target_->branchBooleanType()});
    if (!prevalidated) target_->preflight(mir);
    // Compilation itself is side-effect-free until every output is ready.
    auto built = target_->generate(mir, options_);
    if (built.artifacts.empty()) throw std::runtime_error("NativeBackend: target generated no artifacts");
    std::set<std::string> seen;
    bool hasDeclaredOutput = false;
    for (const auto &item : built.artifacts) {
        if (item.name.empty() || !seen.insert(item.name).second)
            throw std::runtime_error("NativeBackend: empty or duplicate output name");
        if (session.artifacts().has(item.name))
            throw std::runtime_error("NativeBackend: output artifact already exists: " + item.name);
        if (item.target && *item.target != target_->specification())
            throw std::runtime_error("NativeBackend: artifact '" + item.name + "' has a mismatched target");
        // The ELF object/executable contract is only validated for ELF targets;
        // custom opaque outputs remain unrestricted by design.
        if (item.kind == NativeArtifactKind::ObjectFile || item.kind == NativeArtifactKind::Executable) {
            if (target_->specification().objectFormat == "elf64") {
                const auto architecture = target_->specification().architecture;
                // Unknown ELF machine values are not guessed: the provider's
                // architecture-specific metadata is checked when supported.
                std::optional<std::uint16_t> machine;
                if (architecture == "x86-64") machine = 62;
                compilation::object::elf64::verify(item.bytes,
                    item.kind == NativeArtifactKind::ObjectFile ? 1 : 2, machine);
            }
        }
        if (item.name == cap.outputArtifact) {
            if (cap.outputKind && item.kind != *cap.outputKind)
                throw std::runtime_error("NativeBackend: declared output '" + item.name +
                                         "' has the wrong artifact kind");
            hasDeclaredOutput = true;
        }
    }
    if (!hasDeclaredOutput)
        throw std::runtime_error("NativeBackend: target did not produce its declared output '" + cap.outputArtifact + "'");
    std::vector<std::pair<std::string, std::vector<std::uint8_t>>> batch;
    std::vector<compilation::ArtifactStore::Descriptor> descriptors;
    batch.reserve(built.artifacts.size());
    descriptors.reserve(built.artifacts.size());
    for (auto &item : built.artifacts) {
        const char *kind = "opaque";
        switch (item.kind) {
            case NativeArtifactKind::MachineCode: kind = "machine-code"; break;
            case NativeArtifactKind::ObjectFile: kind = "object-file"; break;
            case NativeArtifactKind::Executable: kind = "executable"; break;
            case NativeArtifactKind::Opaque: break;
        }
        descriptors.push_back({kind, target_->specification().display()});
        batch.emplace_back(std::move(item.name), std::move(item.bytes));
    }
    session.artifacts().insertByteBatch(std::move(batch), std::move(descriptors));
}
} // namespace novac::backend
