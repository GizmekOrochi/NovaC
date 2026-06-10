#pragma once

#include "CompilationContext.hpp"
#include "Pass.hpp"

#include "../../../assets/language/Language.hpp"

#include <memory>
#include <string>
#include <vector>

namespace novac::compiler {

class Compiler {
public:
    explicit Compiler(const language::Language &language, std::string startDomain);

    void addPass(std::unique_ptr<Pass> pass);

    template<class T, class... Args>
    void addPass(Args &&...args) {
        addPass(std::make_unique<T>(std::forward<Args>(args)...));
    }

    CompilationContext run(std::string source) const;

private:
    const language::Language &language_;
    std::string startDomain_;
    PassManager passes_;
};

} // namespace novac::compiler