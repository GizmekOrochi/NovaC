#pragma once

#include <string>

namespace novac::token {

enum class Kind {
    Identifier,
    Keyword,
    Symbol,
    Integer,
    End
};

struct Token {
    Kind kind{Kind::End};
    std::string text{};
    int line{1};
    int column{1};
};

} // namespace novac::token