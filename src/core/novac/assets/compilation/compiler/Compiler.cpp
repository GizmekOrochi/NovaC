#include "../../../../../include/novac/assets/compilation/compiler/Compiler.hpp"

#include <utility>

namespace novac::compiler {

Compiler::Compiler(const language::Language &language, std::string startDomain)
    : language_{language}, startDomain_{std::move(startDomain)}, passes_{} {}

void Compiler::addPass(std::unique_ptr<Pass> pass) {
    passes_.add(std::move(pass));
}

CompilationContext Compiler::run(std::string source) const {
    CompilationContext context{language_, startDomain_};

    context.setSource(std::move(source));
    passes_.run(context);

    return context;
}

} // namespace novac::compiler