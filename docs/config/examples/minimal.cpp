#include <iostream>

#include "NovaC.hpp"

int main() {
    novac::controllers::EngineController engine{};
    novac::assets::atomic::AtomicController atomic{engine};

    atomic.integer();
    atomic.add();

    const auto expression{engine.parse("20 + 22")};
    engine.validate(*expression);

    const auto result{engine.eval(*expression)};
    std::cout << result.toString() << '\n';
}
