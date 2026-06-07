#pragma once

#include "CompilationContext.hpp"

#include "../backend/Backend.hpp"

#include <memory>
#include <string>
#include <vector>

namespace novac::compiler {

class Pass {
public:
    virtual ~Pass();
    virtual std::string name() const = 0;
    virtual void run(CompilationContext &context) const = 0;
};

class PassManager {
public:
    PassManager();

    void add(std::unique_ptr<Pass> pass);

    template<class T, class... Args>
    void add(Args &&...args) {
        add(std::make_unique<T>(std::forward<Args>(args)...));
    }

    void run(CompilationContext &context) const;

private:
    std::vector<std::unique_ptr<Pass>> passes_;
};

class ParsePass final : public Pass {
public:
    std::string name() const override;

    void run(CompilationContext &context) const override;
};

class AstValidationPass final : public Pass {
public:
    std::string name() const override;

    void run(CompilationContext &context) const override;
};

class HIRLoweringPass final : public Pass {
public:
    std::string name() const override;

    void run(CompilationContext &context) const override;
};

class MIRLoweringPass final : public Pass {
public:
    std::string name() const override;

    void run(CompilationContext &context) const override;
};

class RuntimePass final : public Pass {
public:
    std::string name() const override;

    void run(CompilationContext &context) const override;
};

class BackendEmitPass final : public Pass {
public:
    explicit BackendEmitPass(std::string backendName);

    std::string name() const override;

    void run(CompilationContext &context) const override;

private:
    std::string backendName_;
};

} // namespace novac::compiler