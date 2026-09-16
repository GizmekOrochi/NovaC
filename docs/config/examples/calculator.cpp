#include <array>
#include <iostream>
#include <string>

#include "NovaC.hpp"

int main() {
    novac::controllers::EngineController engine{};
    novac::assets::atomic::AtomicController atomic{engine};
    atomic.installStandardCore();

    const std::array<std::string, 3> expressions{
        "1 + 2 * 3",
        "10 >= 5",
        "true && !false"
    };

    for(const std::string &source : expressions) {
        const auto expression{engine.parse(source)};
        engine.validate(*expression);

        std::cout << source << " -> "
                  << engine.eval(*expression).toString()
                  << '\n';
    }
}
