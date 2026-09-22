#include <NovaC.hpp>
#include "novac/assets/atomic/literals/IntegerLiteralAtomic.hpp"
#include "novac/assets/atomic/operations/NumericOperations.hpp"

#include <iostream>
#include <vector>

using namespace novac::assets::types;
using novac::assets::atomic::literals::IntegerLiteralAtomic;
using novac::assets::atomic::operations::AddOperationAtomic;

int main() {
    TypeController types;

    types.declareType("u16");
    types.definePrimitive("u8")
        .bits(8)
        .unsignedType()
        .conversionTo(TypeId{"u16"}, ConversionKind::Implicit, 1)
        .commit();

    types.definePrimitive("u16")
        .bits(16)
        .alignment(2)
        .unsignedType()
        .commit();

    types.validate();

    IntegerLiteralAtomic integers;
    types.bindLiteral(integers, TypeId{"u8"});

    AddOperationAtomic add;
    types.registerOperation(
        add,
        {TypeId{"u16"}, TypeId{"u16"}},
        TypeId{"u16"}
    );

    const std::vector<TypeId> operands{TypeId{"u8"}, TypeId{"u16"}};
    const auto result = types.resolveOperationDetailed(add, operands);

    if (!result.ok()) {
        return 1;
    }
    std::cout << result.resolution->result.name << '\n';
    std::cout << result.resolution->conversions.size() << '\n';
}
