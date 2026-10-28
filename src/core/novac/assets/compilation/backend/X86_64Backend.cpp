#include "novac/assets/compilation/backend/X86_64Backend.hpp"
#include "novac/assets/compilation/backend/x86_64/Codegen.hpp"
#include "novac/assets/compilation/backend/x86_64/Elf64Writer.hpp"
#include "novac/assets/compilation/backend/x86_64/StandardRecipes.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>

namespace novac::backend {
X86_64Backend::X86_64Backend(x86_64::ValueTypes types,
     compilation::machine::MachineRecipeRegistry custom,
     compilation::machine::MachineTarget target)
    : types_{std::move(types)}, recipes_{x86_64::standardRecipes(types_)}, target_{std::move(target)} {
    if (target_.architecture != "x86-64")
        throw std::runtime_error("X86_64Backend: unsupported target '" + target_.architecture + "'");
    recipes_.append(custom);
    recipes_.freeze();
}
void X86_64Backend::preflight(const ir::MIRModule &mir) const {
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
    for (std::size_t b=0;b<mir.blocks.size();++b) {
        for (std::size_t i=0;i<mir.blocks[b].instructions.size();++i) {
            const auto &inst=mir.blocks[b].instructions[i];
            try {
                (void)recipes_.select(inst,target_,[&](ir::ValueId id)->std::optional<ir::TypeRef>{
                    const auto it=definitions.find(id.value);
                    return it==definitions.end()?std::nullopt:std::optional<ir::TypeRef>{it->second};
                });
            } catch (const std::exception &e) {
                throw std::runtime_error("X86_64Backend preflight: block "+std::to_string(b)+
                    ", instruction "+std::to_string(i)+" '"+inst.op+"': "+e.what());
            }
        }
    }
}
void X86_64Backend::run(compilation::CompilationSession &session) const {
    const auto &mir = session.artifacts().require<ir::MIRModule>("mir");
    for (const char *name : {"x86-64-machine-code", "x86-64-elf-object", "x86-64-elf-executable"}) {
        if (session.artifacts().has(name))
            throw std::runtime_error(std::string("X86_64Backend: output artifact already exists: ") + name);
    }
    const auto image = x86_64::selectAndEncode(mir, types_, recipes_, target_);
    auto object = x86_64::makeObject(image.text, image.mainOffset);
    auto executable = x86_64::makeExecutable(image.text);
    // All outputs are built before any insertion, so unsupported MIR never
    // leaves a partial set of artifacts in the session.
    session.artifacts().insert<std::vector<std::uint8_t>>("x86-64-machine-code", image.text);
    session.artifacts().insert<std::vector<std::uint8_t>>("x86-64-elf-object", std::move(object));
    session.artifacts().insert<std::vector<std::uint8_t>>("x86-64-elf-executable", std::move(executable));
}
} // namespace novac::backend
