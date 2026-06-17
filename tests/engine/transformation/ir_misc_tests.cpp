#include "../../tester.hpp"
#include "novac/engine/transformation/IR.hpp"

using novac::ir::ValueType;

TEST(ValueType, ConvertsToStableNames) {
    CHECK(novac::ir::valueTypeName(ValueType::Unknown) == "unknown");
    CHECK(novac::ir::valueTypeName(ValueType::Void) == "void");
    CHECK(novac::ir::valueTypeName(ValueType::Int) == "int");
    CHECK(novac::ir::valueTypeName(ValueType::Bool) == "bool");
    CHECK(novac::ir::valueTypeName(ValueType::Float) == "float");
    CHECK(novac::ir::valueTypeName(ValueType::String) == "string");
}

TEST(ValueId, DefaultConstruction) {
    novac::ir::ValueId id{};

    CHECK(id.value == 0);
}

TEST(BlockId, DefaultConstruction) {
    novac::ir::BlockId id{};

    CHECK(id.value == 0);
}

TEST(Instruction, DefaultConstruction) {
    novac::ir::Instruction instruction{};

    CHECK(instruction.op.empty());
    CHECK(!instruction.result.has_value());
    CHECK(instruction.operands.empty());
    CHECK(instruction.type == ValueType::Unknown);
}

TEST(Terminator, DefaultConstruction) {
    novac::ir::Terminator terminator{};

    CHECK(terminator.op.empty());
    CHECK(terminator.operands.empty());
    CHECK(terminator.targets.empty());
}

TEST(BasicBlock, DefaultConstruction) {
    novac::ir::BasicBlock block{};

    CHECK(block.id.value == 0);
    CHECK(block.name.empty());
    CHECK(block.instructions.empty());
    CHECK(!block.terminator.has_value());
}
