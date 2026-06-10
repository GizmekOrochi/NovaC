#pragma once

#include "CompilationContext.hpp"

#include "novac/assets/semantic/Semantic.hpp"
#include "novac/engine/syntax/Lexer.hpp"
#include "novac/engine/syntax/Parser.hpp"
#include "novac/engine/execution/Runtime.hpp"

#include <memory>
#include <string>
#include <utility>
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
    explicit ParsePass(std::string outputArtifact = "ast");

    std::string name() const override;

    void run(CompilationContext &context) const override;

private:
    std::string outputArtifact_;
};

class AstValidationPass final : public Pass {
public:
    explicit AstValidationPass(std::string astArtifact = "ast");

    std::string name() const override;

    void run(CompilationContext &context) const override;

private:
    std::string astArtifact_;
};

class SemanticPass final : public Pass {
public:
    SemanticPass(
        std::string inputAst = "ast",
        std::string outputSemantic = "semantic");

    std::string name() const override;

    void run(CompilationContext &context) const override;

private:
    std::string inputAst_;
    std::string outputSemantic_;
};

class HIRLoweringPass final : public Pass {
public:
    HIRLoweringPass(std::string inputAst = "ast", std::string outputHir = "hir");

    std::string name() const override;

    void run(CompilationContext &context) const override;

private:
    std::string inputAst_;
    std::string outputHir_;
};

class MIRLoweringPass final : public Pass {
public:
    MIRLoweringPass(std::string inputHir = "hir", std::string outputMir = "mir");

    std::string name() const override;

    void run(CompilationContext &context) const override;

private:
    std::string inputHir_;
    std::string outputMir_;
};

class RuntimePass final : public Pass {
public:
    RuntimePass(std::string inputAst = "ast", std::string outputValue = "result");

    std::string name() const override;

    void run(CompilationContext &context) const override;

private:
    std::string inputAst_;
    std::string outputValue_;
};

} // namespace novac::compiler