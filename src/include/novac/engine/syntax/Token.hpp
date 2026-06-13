#pragma once

#include "../foundation/Diagnostic.hpp"

#include <string>

namespace novac::token {

enum class Kind {
    Identifier,
    Keyword,
    Symbol,
    Integer,
    Float,
    String,
    End
};

struct Token {
    Kind kind{Kind::End};
    std::string text{};
    std::string suffix{};
    diagnostics::SourceSpan span{};

    int line{1};
    int column{1};
};

} // namespace novac::token