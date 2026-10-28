#include "novac/assets/compilation/backend/x86_64/StandardRecipes.hpp"

namespace novac::backend::x86_64 {
compilation::machine::MachineRecipeRegistry standardRecipes(const ValueTypes &types) {
    using namespace compilation::machine;
    MachineRecipeRegistry registry;
    const auto add=[&](std::string id, std::string op, ir::TypeRef type, RecipeEmitter emitter) {
        MachineRecipe r{};
        r.id=std::move(id);r.operation=std::move(op);r.resultType=std::move(type);
        r.emit=std::move(emitter);registry.add(std::move(r));
    };
    add("x64.standard.i32.const", "novac.i32.const",types.i32,
        [](const ir::Instruction &inst, MachineBuilder &b) {
            if (inst.operands.size()!=1) throw std::runtime_error("i32 const arity");
            b.loadIntegerLiteral();
        });
    add("x64.standard.bool.const", "novac.bool.const",types.boolean,
        [](const ir::Instruction &inst, MachineBuilder &b) {
            if (inst.operands.size()!=1) throw std::runtime_error("bool const arity");
            b.loadBooleanLiteral();
        });
    const auto binary=[&](std::string id, std::string op, ir::TypeRef type,
                          void (MachineBuilder::*action)(std::size_t)) {
        MachineRecipe r{};
        r.id=std::move(id); r.operation=std::move(op); r.resultType=std::move(type);
        r.operandTypes=std::vector<ir::TypeRef>{types.i32, types.i32};
        r.emit=[action](const ir::Instruction &inst,MachineBuilder &b) {
            if (inst.operands.size()!=2) throw std::runtime_error("binary operation requires 2 operands");
            b.loadInput(0);(b.*action)(1);
        };
        registry.add(std::move(r));
    };
    binary("x64.standard.i32.add","novac.i32.add",types.i32,&MachineBuilder::addInput);
    binary("x64.standard.i32.sub","novac.i32.sub",types.i32,&MachineBuilder::subInput);
    binary("x64.standard.i32.mul","novac.i32.mul",types.i32,&MachineBuilder::mulInput);
    binary("x64.standard.i32.lt","novac.i32.lt",types.boolean,&MachineBuilder::signedLessThanInput);
    MachineRecipe noop{};
    noop.id="x64.standard.machine.nop";
    noop.owner="novac.x86-64";
    noop.operation="novac.machine.nop";
    noop.operandPatterns=std::vector<OperandPattern>{};
    noop.emit=[](const ir::Instruction &,MachineBuilder &b){ b.noOp(); };
    registry.add(std::move(noop));
    return registry;
}
} // namespace novac::backend::x86_64

namespace novac::backend::x86_64 {
void installScalarRecipes(compilation::machine::MachineRecipeRegistry &registry,
                          const ir::TypeRef &type, compilation::machine::MachineLayout layout,
                          const std::string &prefix, ir::TypeRef booleanType) {
    using namespace compilation::machine;
    layout.validate();
    if (layout.kind==ScalarKind::Boolean)
        throw std::runtime_error("installScalarRecipes: boolean is not an integer scalar");
    const auto add=[&](std::string name, std::vector<ir::TypeRef> inputs,
                       ir::TypeRef result, RecipeEmitter emitter, bool literal=false) {
        MachineRecipe recipe{};
        recipe.id="x64.scalar."+prefix+"."+name;
        recipe.owner="novac.x86-64.scalar";
        recipe.operation="novac."+prefix+"."+name;
        recipe.resultType=std::move(result);
        if (literal) recipe.operandPatterns=std::vector<OperandPattern>{{OperandKind::IntegerLiteral,{}}};
        else recipe.operandTypes=std::move(inputs);
        recipe.emit=std::move(emitter);
        registry.add(std::move(recipe));
    };
    add("const",{},type,[](const ir::Instruction &, MachineBuilder &b){ b.loadIntegerLiteral(); },true);
    add("add",{type,type},type,[](const ir::Instruction &,MachineBuilder &b){b.loadInput(0);b.addInput(1);});
    add("sub",{type,type},type,[](const ir::Instruction &,MachineBuilder &b){b.loadInput(0);b.subInput(1);});
    add("mul",{type,type},type,[](const ir::Instruction &,MachineBuilder &b){b.loadInput(0);b.mulInput(1);});
    add("lt",{type,type},std::move(booleanType),[layout](const ir::Instruction &,MachineBuilder &b){
        b.loadInput(0);
        if(layout.kind==ScalarKind::SignedInteger) b.signedLessThanInput(1);
        else b.unsignedLessThanInput(1);
    });
}
} // namespace novac::backend::x86_64
