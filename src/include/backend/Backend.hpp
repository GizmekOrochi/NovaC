#pragma once

#include "../ir/IR.hpp"
#include "../registry/Registry.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <utility>

namespace novac::backend {

class Backend {
public:
    virtual ~Backend();
    virtual std::string name() const = 0;
    virtual void emit(const ir::MIRModule &module) = 0;
};

class InterpreterBackend final : public Backend {
public:
    std::string name() const override;

    void emit(const ir::MIRModule &module) override;
};

class BytecodeBackend final : public Backend {
public:
    std::string name() const override;

    void emit(const ir::MIRModule &module) override;
};

class LLVMBackend final : public Backend {
public:
    std::string name() const override;

    void emit(const ir::MIRModule &module) override;
};

class CTranspilerBackend final : public Backend {
public:
    std::string name() const override;

    void emit(const ir::MIRModule &module) override;
};

class BackendRegistry {
public:
    explicit BackendRegistry(
        registry::DuplicatePolicy duplicatePolicy = registry::DuplicatePolicy::Error);

    registry::RegisterStatus add(
        std::string name,
        std::function<std::unique_ptr<Backend>()> factory);

    std::unique_ptr<Backend> create(const std::string &name) const;

private:
    std::unordered_map<std::string, std::function<std::unique_ptr<Backend>()>> factories_;
    registry::DuplicatePolicy duplicatePolicy_;
};

class BackendPipeline {
public:
    BackendPipeline(
        const BackendRegistry &backends,
        const ir::LoweringRegistry &lowering);

    bool run(const ast::Node &ast, const std::string &backendName) const;

private:
    const BackendRegistry &backends_;
    const ir::LoweringRegistry &lowering_;
};

} // namespace novac::backend