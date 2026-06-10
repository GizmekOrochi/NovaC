#pragma once

#include <string>
#include <type_traits>

namespace novac::compiler {
class CompilationContext;
} // namespace novac::compiler

namespace novac::controllers {

class ArtifactController final {
public:
    explicit ArtifactController(compiler::CompilationContext &context);

    ArtifactController(const ArtifactController &) = delete;
    ArtifactController &operator=(const ArtifactController &) = delete;

    ArtifactController(ArtifactController &&other) noexcept = default;
    ArtifactController &operator=(ArtifactController &&other) noexcept = default;

    template<typename T>
    T &require(const std::string &name) const
    {
        static_assert(!std::is_reference_v<T>, "ArtifactController::require: T must not be a reference");

        return requireImpl<T>(name);
    }

    compiler::CompilationContext &context() const;

private:
    template<typename T>
    T &requireImpl(const std::string &name) const;

    compiler::CompilationContext *context_;
};

} // namespace novac::controllers

#include "novac/engine/compilation/compiler/CompilationContext.hpp"

namespace novac::controllers {

template<typename T>
T &ArtifactController::requireImpl(const std::string &name) const
{
    return context().requireArtifact<T>(name);
}

} // namespace novac::controllers
