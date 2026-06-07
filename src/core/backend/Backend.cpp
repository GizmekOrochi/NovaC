#include "../../include/backend/Backend.hpp"

#include <utility>

namespace novac::backend {

Backend::~Backend() = default;

std::string InterpreterBackend::name() const {
    return "interpreter";
}

void InterpreterBackend::emit(const ir::MIRModule &module) {
    static_cast<void>(module);
}

std::string BytecodeBackend::name() const {
    return "bytecode";
}

void BytecodeBackend::emit(const ir::MIRModule &module) {
    static_cast<void>(module);
}

std::string LLVMBackend::name() const {
    return "llvm";
}

void LLVMBackend::emit(const ir::MIRModule &module) {
    static_cast<void>(module);
}

std::string CTranspilerBackend::name() const {
    return "c";
}

void CTranspilerBackend::emit(const ir::MIRModule &module) {
    static_cast<void>(module);
}

BackendRegistry::BackendRegistry(registry::DuplicatePolicy duplicatePolicy)
    : factories_{}, duplicatePolicy_{duplicatePolicy} {}

registry::RegisterStatus BackendRegistry::add(std::string name, std::function<std::unique_ptr<Backend>()> factory) {
    const auto iter{factories_.find(name)};

    if (iter != factories_.end()) {
        if (duplicatePolicy_ == registry::DuplicatePolicy::Ignore) {
            return registry::RegisterStatus::Ignored;
        }

        if (duplicatePolicy_ == registry::DuplicatePolicy::Replace) {
            iter->second = std::move(factory);

            return registry::RegisterStatus::Replaced;
        }

        throw std::runtime_error(
            "BackendRegistry::add: duplicate backend '" + name + "'");
    }

    factories_.emplace(std::move(name), std::move(factory));

    return registry::RegisterStatus::Inserted;
}

BackendPipeline::BackendPipeline(const BackendRegistry &backends, const ir::LoweringRegistry &lowering) 
    : backends_{backends}, lowering_{lowering} {}

bool BackendPipeline::run(const ast::Node &ast, const std::string &backendName) const {
    ir::ASTLoweringPass astLower{lowering_};
    ir::HIRLoweringPass hirLower{lowering_};

    const auto hir{astLower.lower(ast)};
    const auto mir{hirLower.lower(hir)};

    const std::unique_ptr<Backend> backend{backends_.create(backendName)};

    if (!backend) {
        return false;
    }

    backend->emit(mir);

    return true;
}

std::unique_ptr<Backend> BackendRegistry::create(const std::string &name) const {
    const auto iter{factories_.find(name)};

    if (iter == factories_.end()) {
        return nullptr;
    }

    return iter->second();
}

} // namespace novac::backend