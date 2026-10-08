#include <iostream>

#include <NovaC.hpp>

int main() {
    novac::controllers::EngineController engine{};
    novac::assets::atomic::AtomicController atomic{engine};

    atomic.integer();
    atomic.add();

    const std::string source{R"(20 + 22)"};
    const auto expression{engine.parse(source)};
    engine.validate(*expression);

    const auto result{engine.eval(*expression)};
    std::cout << result.toString() << '\n';
}
