#include <sstream>
#include <iostream>
#include <string>

#include <NovaC.hpp>

int main() {
    novac::controllers::EngineController engine{};
    novac::assets::atomic::AtomicController atomic{engine};
    atomic.installStandardCore();

    const std::string source{R"(
1 + 2 * 3
10 >= 5
true && !false
)"};

    // Each nonempty line is an independent expression in this calculator dialect.
    std::istringstream lines{source};
    std::string expressionText;
    while (std::getline(lines, expressionText)) {
        if (expressionText.empty()) continue;
        const auto expression{engine.parse(expressionText)};
        engine.validate(*expression);

        std::cout << expressionText << " -> " << engine.eval(*expression).toString() << '\n';
    }
}
