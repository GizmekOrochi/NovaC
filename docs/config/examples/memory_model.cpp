#include <NovaC.hpp>
#include <string>

#include <iostream>
#include <memory>

using namespace novac::assets::memory;
using namespace novac::assets::types;

int main() {
    // Illustrative syntax for the values represented by the memory allocations.
    // Allocation and lifetime are demonstrated through the API, not a parser.
    const std::string source{R"(
CardinalDirection a = 0;
CardinalDirection b = 1;
CardinalDirection c = 2;
CardinalDirection d = 3;
)"};
    (void)source;

    TypeController types;
    types.definePrimitive("CardinalDirection")
        .bits(2)
        .alignmentBits(1)
        .unsignedType()
        .commit();

    LayoutController layouts{types};
    StorageController storage{types, layouts};
    MemoryController memory{types, layouts, storage};

    const auto ram{memory.createAddressSpace("ram", 16, std::make_unique<BitStorageAccess>(16))};
    const auto all{memory.createRegion("all", AddressRange{Address{ram, 0}, 16})};

    const auto a{memory.allocate(all, TypeId{"CardinalDirection"})};
    const auto b{memory.allocate(all, TypeId{"CardinalDirection"})};
    const auto c{memory.allocate(all, TypeId{"CardinalDirection"})};
    const auto d{memory.allocate(all, TypeId{"CardinalDirection"})};

    memory.store(a, BitValue::fromUnsigned(0, 2));
    memory.store(b, BitValue::fromUnsigned(1, 2));
    memory.store(c, BitValue::fromUnsigned(2, 2));
    memory.store(d, BitValue::fromUnsigned(3, 2));

    std::cout << "offsets=" << a.address.bitOffset << ',' << b.address.bitOffset << ',' << c.address.bitOffset << ',' << d.address.bitOffset << '\n';
    std::cout << "packed=" << memory.loadBits(Address{ram, 0}, 8).toUnsigned() << '\n';

    const auto life{memory.beginLifetime()};
    const auto managed{memory.allocate(all, TypeId{"CardinalDirection"}, life)};
    memory.store(managed, BitValue::fromUnsigned(1, 2));
    memory.endLifetime(life);

    try {
        (void)memory.load(managed);
    } catch (const MemoryError &error) {
        std::cout << "managed-after-end=" << (error.code() == MemoryErrorCode::DeadLifetime ? "dead" : "other") << '\n';
    }
}
