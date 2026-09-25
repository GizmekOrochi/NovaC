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
        .alignmentBytes(2)
        .unsignedType()
        .commit();

    types.registerAlias("word", TypeId{"u16"});

    IntegerLiteralAtomic integers;
    types.bindLiteral(integers, TypeId{"u8"});

    AddOperationAtomic add;
    types.registerOperation(add, {TypeId{"word"}, TypeId{"word"}}, TypeId{"word"});

    types.finalize();

    const std::vector<TypeId> operands{TypeId{"u8"}, TypeId{"word"}};
    const auto result{types.resolveOperationDetailed(add, operands)};

    if (!result.ok()) {
        std::cerr << result.diagnostic.message << std::endl;
        return 1;
    }

    std::cout << result.resolution->result.name << std::endl;
    std::cout << result.resolution->conversions.size() << std::endl;
}
