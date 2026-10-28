#include "../../tester.hpp"
#include "novac/engine/transformation/IR.hpp"
#include <stdexcept>
#include <string>

TEST(IRValidation, CustomBranchBooleanMustBeDeclaredExplicitly) {
    using namespace novac::ir;
    const TypeRef flag{"truth","my.language"};
    MIRBuilder builder;
    const auto yes=builder.createBlock("yes"),no=builder.createBlock("no");
    const auto cond=builder.emitValue("custom.bool.const",{Operand::fromLiteral(true)},flag);
    builder.terminateWithSuccessors("branch",{Operand::fromValue(cond)},{{yes,{}},{no,{}}});
    builder.setCurrentBlock(yes);builder.terminate("return");
    builder.setCurrentBlock(no);builder.terminate("return");
    bool rejected=false;
    try { (void)builder.finish(); } catch (const std::runtime_error &error) {
        rejected=std::string(error.what()).find("branch condition")!=std::string::npos;
    }
    CHECK(rejected);
    auto module=builder.finish({.branchBooleanType=flag});
    CHECK(module.blocks.size()==3);
    validate(module, {.requireTerminators=true,.branchBooleanType=flag});
}
