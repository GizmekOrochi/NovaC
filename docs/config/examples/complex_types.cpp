#include <NovaC.hpp>
#include "novac/assets/types/aggregate/StructType.hpp"

#include <iostream>

using namespace novac::assets::types;
using namespace novac::assets::types::aggregate;

int main() {
    TypeController types;

    types.definePrimitive("short")
        .bits(16)
        .storageBits(16)
        .alignment(2)
        .signedType()
        .commit();

    types.definePrimitive("int")
        .bits(32)
        .storageBits(32)
        .alignment(4)
        .signedType()
        .commit();

    defineStruct(types, "Example")
        .field("a", TypeId{"short"})
        .field("b", TypeId{"int"})
        .field("c", TypeId{"short"})
        .commit();

    types.finalize();

    LayoutController layouts{types};
    const TypeLayout layout{layouts.compute(TypeId{"Example"})};

    const TypeDefinition &example{types.requireType(TypeId{"Example"})};
    const auto *members{example.capabilities.get<MemberCapability>()};
    if (!members) {
        std::cerr << "Example has no member capability" << std::endl;
        return 1;
    }

    const auto member{members->findMember(example, "b")};
    if (!member) {
        std::cerr << "member b was not found" << std::endl;
        return 1;
    }

    std::cout << example.id.name << std::endl;
    std::cout << "size=" << layout.size << " align=" << layout.alignment << std::endl;

    for (const auto &component : layout.components)
        std::cout << component.componentIndex << ':' << component.offset << std::endl;

    std::cout << "b:" << member->type.name << std::endl;
}
