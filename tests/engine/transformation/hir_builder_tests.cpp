#include "../../tester.hpp"
#include "novac/engine/transformation/IR.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace {

using novac::ir::BlockId;
using novac::ir::HIRBuilder;
using novac::ir::Operand;
using novac::ir::ValueType;

template <typename Fn>
bool throwsRuntimeError(Fn &&fn) {
    try {
        fn();
    } catch (const std::runtime_error &) {
        return true;
    } catch (...) {
        return false;
    }

    return false;
}

} // namespace

TEST(HIRBuilder, StartsWithEntryBlock) {
    HIRBuilder builder{};
    auto module{builder.finish()};

    CHECK(module.blocks.size() == 1);
    CHECK(module.blocks[0].id.value == 0);
    CHECK(module.blocks[0].name == "entry");
    CHECK(module.blocks[0].instructions.empty());
    CHECK(!module.blocks[0].terminator.has_value());
}

TEST(HIRBuilder, CreatesBlocks) {
    HIRBuilder builder{};

    BlockId first{builder.createBlock("then")};
    BlockId second{builder.createBlock("else")};

    CHECK(first.value == 1);
    CHECK(second.value == 2);

    auto module{builder.finish()};

    CHECK(module.blocks.size() == 3);
    CHECK(module.blocks[1].name == "then");
    CHECK(module.blocks[2].name == "else");
}

TEST(HIRBuilder, EmitsValueInstruction) {
    HIRBuilder builder{};

    auto value{builder.emitValue("const", {Operand::fromLiteral(42)}, ValueType::Int)};

    CHECK(value.value == 0);

    auto module{builder.finish()};

    CHECK(module.blocks[0].instructions.size() == 1);
    CHECK(module.blocks[0].instructions[0].op == "const");
    CHECK(module.blocks[0].instructions[0].result.has_value());
    CHECK(module.blocks[0].instructions[0].result->value == 0);
    CHECK(module.blocks[0].instructions[0].type == ValueType::Int);
    CHECK(module.blocks[0].instructions[0].operands.size() == 1);
}

TEST(HIRBuilder, EmitsValueInstructionWithTypedOperation) {
    HIRBuilder builder{};
    novac::ids::Operation op{"const"};

    auto value{builder.emitValue(op, {}, ValueType::Int)};

    CHECK(value.value == 0);

    auto module{builder.finish()};
    CHECK(module.blocks[0].instructions[0].op == "const");
}

TEST(HIRBuilder, EmitsVoidInstruction) {
    HIRBuilder builder{};

    builder.emit("store", {Operand::fromSymbol("x")});

    auto module{builder.finish()};

    CHECK(module.blocks[0].instructions.size() == 1);
    CHECK(module.blocks[0].instructions[0].op == "store");
    CHECK(!module.blocks[0].instructions[0].result.has_value());
    CHECK(module.blocks[0].instructions[0].type == ValueType::Void);
}

TEST(HIRBuilder, EmitsVoidInstructionWithTypedOperation) {
    HIRBuilder builder{};
    novac::ids::Operation op{"store"};

    builder.emit(op);

    auto module{builder.finish()};
    CHECK(module.blocks[0].instructions[0].op == "store");
}

TEST(HIRBuilder, ChangesCurrentBlock) {
    HIRBuilder builder{};

    BlockId second{builder.createBlock("second")};
    builder.setCurrentBlock(second);
    builder.emit("mark");

    auto module{builder.finish()};

    CHECK(module.blocks[0].instructions.empty());
    CHECK(module.blocks[1].instructions.size() == 1);
    CHECK(module.blocks[1].instructions[0].op == "mark");
}

TEST(HIRBuilder, RejectsInvalidCurrentBlock) {
    HIRBuilder builder{};

    CHECK(throwsRuntimeError([&]() { builder.setCurrentBlock(BlockId{99}); }));
}

TEST(HIRBuilder, TerminatesCurrentBlock) {
    HIRBuilder builder{};
    BlockId target{builder.createBlock("exit")};

    builder.terminate("jump", {}, {target});

    auto module{builder.finish()};

    CHECK(module.blocks[0].terminator.has_value());
    CHECK(module.blocks[0].terminator->op == "jump");
    CHECK(module.blocks[0].terminator->targets.size() == 1);
    CHECK(module.blocks[0].terminator->targets[0].value == target.value);
}

TEST(HIRBuilder, TerminatesCurrentBlockWithTypedOperation) {
    HIRBuilder builder{};
    novac::ids::Operation op{"return"};

    builder.terminate(op);

    auto module{builder.finish()};
    CHECK(module.blocks[0].terminator->op == "return");
}

TEST(HIRBuilder, RejectsInvalidTerminators) {
    HIRBuilder builder{};

    CHECK(throwsRuntimeError([&]() { builder.terminate(""); }));

    builder.terminate("return");

    CHECK(throwsRuntimeError([&]() { builder.terminate("return"); }));
}

TEST(HIRBuilder, RejectsEmitAfterTerminator) {
    HIRBuilder builder{};

    builder.terminate("return");

    CHECK(throwsRuntimeError([&]() { builder.emit("after"); }));
    CHECK(throwsRuntimeError([&]() { static_cast<void>(builder.emitValue("after")); }));
}
