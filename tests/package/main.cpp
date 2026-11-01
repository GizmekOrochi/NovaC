#include <NovaC.hpp>
#include <string_view>

int main() {
    static_assert(NOVAC_VERSION_MAJOR == 1);
    static_assert(NOVAC_VERSION_MINOR == 4);
    static_assert(NOVAC_VERSION_PATCH == 2);
    static_assert(novac::Version == std::string_view{"1.4.2"});
    novac::controllers::EngineController engine;
    (void)engine;
    return 0;
}
