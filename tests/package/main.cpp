#include <NovaC.hpp>

#include <string_view>

int main() {
    static_assert(NOVAC_VERSION_MAJOR == 1);
    static_assert(NOVAC_VERSION_MINOR == 4);
    static_assert(NOVAC_VERSION_PATCH == 0);
    static_assert(novac::Version == std::string_view{"1.4.0"});

    novac::controllers::EngineController engine;
    novac::assets::types::TypeController types;
    novac::assets::types::LayoutController layouts{types};
    novac::assets::types::StorageController storage{types, layouts};
    novac::assets::memory::MemoryController memory{types, layouts, storage};
    (void)engine;
    (void)memory;
    return 0;
}
