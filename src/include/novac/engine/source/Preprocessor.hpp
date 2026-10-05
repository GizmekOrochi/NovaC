#pragma once

#include "Source.hpp"

#include <string>

namespace novac::source {

/** @brief Parsed source directive passed to extension handlers. */
struct Directive {
    /** Directive name without the configured prefix. */
    std::string name{};

    /** Trimmed directive arguments. For pragma handlers, the pragma name is removed. */
    std::string arguments{};

    /** Span covering the directive from its prefix through its physical content. */
    diagnostics::SourceSpan span{};

    /** Span covering the dispatched argument region. */
    diagnostics::SourceSpan argumentsSpan{};
};

/**
 * @brief Controls whether a directive runs while its conditional branch is inactive.
 */
enum class DirectiveMode {
    /** Run only while the current conditional branch is active. */
    ActiveOnly,

    /** Run regardless of the current conditional state. */
    Always
};

} // namespace novac::source
