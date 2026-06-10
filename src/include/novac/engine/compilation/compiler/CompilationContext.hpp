#pragma once

#include "../../foundation/Diagnostic.hpp"
#include "../../../assets/language/Language.hpp"

#include <any>
#include <string>
#include <unordered_map>
#include <utility>
#include <stdexcept>

namespace novac::compiler {

class CompilationContext {
public:
    CompilationContext(const language::Language &language, std::string startDomain);

    const language::Language &language() const;
    const std::string &startDomain() const;

    void setSource(std::string source);
    const std::string &source() const;

    template<typename T>
    void setArtifact(std::string name, T value) {
        artifacts_[std::move(name)] = std::move(value);
    }

    template<typename T>
    bool hasArtifact(const std::string &name) const {
        return artifacts_.find(name) != artifacts_.end();
    }

    template<typename T>
    T &requireArtifact(const std::string &name) {
        const auto iter{artifacts_.find(name)};

        if (iter == artifacts_.end())
            throw std::runtime_error("CompilationContext::requireArtifact: missing artifact '" + name + "'");

        return std::any_cast<T &>(iter->second);
    }

    template<typename T>
    const T &requireArtifact(const std::string &name) const {
        const auto iter{artifacts_.find(name)};

        if (iter == artifacts_.end())
            throw std::runtime_error("CompilationContext::requireArtifact: missing artifact '" + name + "'");

        return std::any_cast<const T &>(iter->second);
    }

    diagnostics::DiagnosticEngine &diagnostics();
    const diagnostics::DiagnosticEngine &diagnostics() const;

private:
    const language::Language &language_;
    std::string startDomain_;
    std::string source_;

    std::unordered_map<std::string, std::any> artifacts_;

    diagnostics::DiagnosticEngine diagnostics_;
};

} // namespace novac::compiler