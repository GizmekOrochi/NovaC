#pragma once

#include <memory>
#include <string>

namespace novac::compiler {
class CompilationContext;
class Pass;
} // namespace novac::compiler

namespace novac::controllers {

class LanguageController;

struct CompilationArtifacts final {
    std::string ast{"ast"};
    std::string semantic{"semantic"};
    std::string hir{"hir"};
    std::string mir{"mir"};
    std::string result{"result"};
};

class CompilerController final {
public:
    CompilerController(
        const LanguageController &language,
        std::string startDomain);

    ~CompilerController();

    CompilerController(const CompilerController &) = delete;
    CompilerController &operator=(const CompilerController &) = delete;

    CompilerController(CompilerController &&other) noexcept;
    CompilerController &operator=(CompilerController &&other) noexcept;

    void addPass(std::unique_ptr<compiler::Pass> pass);
    void addCorePipeline(const CompilationArtifacts &artifacts = CompilationArtifacts{});
    void addExecutionPipeline(const CompilationArtifacts &artifacts = CompilationArtifacts{});
    void addFullPipeline(const CompilationArtifacts &artifacts = CompilationArtifacts{});

    std::unique_ptr<compiler::CompilationContext> run(std::string source) const;

private:
    struct Impl;

    std::unique_ptr<Impl> impl_;
};

} // namespace novac::controllers
