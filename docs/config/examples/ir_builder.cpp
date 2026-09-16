#include <iostream>

#include "NovaC.hpp"
#include "novac/engine/transformation/IR.hpp"

int main() {
    novac::ir::HIRBuilder builder{};

    const novac::ir::ValueId left{
        builder.emitValue(
            "const",
            {novac::ir::Operand::fromLiteral(20)},
            novac::ir::ValueType::Int
        )
    };

    const novac::ir::ValueId right{
        builder.emitValue(
            "const",
            {novac::ir::Operand::fromLiteral(22)},
            novac::ir::ValueType::Int
        )
    };

    const novac::ir::ValueId sum{
        builder.emitValue(
            "add",
            {
                novac::ir::Operand::fromValue(left),
                novac::ir::Operand::fromValue(right)
            },
            novac::ir::ValueType::Int
        )
    };

    builder.terminate("return", {novac::ir::Operand::fromValue(sum)});

    const novac::ir::HIRModule module{builder.finish()};

    for(const novac::ir::BasicBlock &block : module.blocks) {
        std::cout << block.name << '\n';
        for(const novac::ir::Instruction &instruction : block.instructions)
            std::cout << "  " << instruction.op << '\n';
        if(block.terminator)
            std::cout << "  " << block.terminator->op << '\n';
    }
}
