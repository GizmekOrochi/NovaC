#include "../../tester.hpp"
#include "novac/assets/compilation/machine/MachineRecipe.hpp"
#include "novac/assets/compilation/backend/x86_64/Codegen.hpp"
#include "novac/assets/compilation/backend/x86_64/StandardRecipes.hpp"
#include "novac/assets/compilation/backend/X86_64Backend.hpp"
#include "novac/assets/compilation/backend/x86_64/Elf64Writer.hpp"
#include "novac/assets/atomic/compilation/AtomicMachineRecipes.hpp"
#include "novac/assets/atomic/compilation/AtomicCompilationPack.hpp"
#include "novac/assets/types/compilation/TypeMachineRecipes.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#if defined(__linux__) && defined(__x86_64__)
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#endif

namespace {
using namespace novac;
using namespace novac::compilation::machine;
using O = ir::Operand;
const ir::TypeRef I32{"i32","novac.prototype"};
template<class F> std::string failure(F &&f) {
    try { f(); } catch (const std::runtime_error &e) { return e.what(); }
    return {};
}
ir::MIRModule arithmetic(std::string name, int a, int b) {
    ir::MIRBuilder mir;
    auto x=mir.emitValue("novac.i32.const", {O::fromLiteral(a)}, I32);
    auto y=mir.emitValue("novac.i32.const", {O::fromLiteral(b)}, I32);
    auto result=mir.emitValue(std::move(name),{O::fromValue(x),O::fromValue(y)},I32);
    mir.terminate("return",{O::fromValue(result)});
    return mir.finish();
}
ir::MIRModule single(std::string operation, const ir::TypeRef &type, int literal) {
    ir::MIRBuilder mir;
    auto value=mir.emitValue(std::move(operation),{O::fromLiteral(literal)},type);
    mir.terminate("return",{O::fromValue(value)});
    return mir.finish();
}
#if defined(__linux__) && defined(__x86_64__)
int runNative(const std::vector<std::uint8_t> &bytes) {
    char path[] = "/tmp/novac-recipe-XXXXXX";
    const int fd=mkstemp(path);
    if (fd<0) throw std::runtime_error("cannot create temporary ELF");
    if (fchmod(fd, 0700)!=0) { close(fd); unlink(path); throw std::runtime_error("cannot chmod ELF"); }
    std::size_t position=0;
    while (position<bytes.size()) {
        const ssize_t n=write(fd,bytes.data()+position,bytes.size()-position);
        if (n<=0) { close(fd); unlink(path); throw std::runtime_error("cannot write ELF"); }
        position+=static_cast<std::size_t>(n);
    }
    close(fd);
    const pid_t pid=fork();
    if (pid<0) {unlink(path);throw std::runtime_error("fork failed");}
    if (pid==0) {execl(path,"novac-recipe-test",static_cast<char*>(nullptr)); _exit(252);}
    int status=0;
    const pid_t waited=waitpid(pid,&status,0);
    unlink(path);
    if (waited!=pid || !WIFEXITED(status)) throw std::runtime_error("native ELF failed to exit normally");
    return WEXITSTATUS(status);
}
#endif
MachineRecipe makeRule(std::string name, std::string operation, const ir::TypeRef &type) {
    MachineRecipe r{};
    r.id=std::move(name);r.operation=std::move(operation);r.resultType=type;
    r.tier=RecipeTier::Override;
    r.emit=[](const ir::Instruction &, MachineBuilder &b){b.loadImmediate(42);};
    return r;
}
}

TEST(MachineRecipes, AtomicPackAddsNewOperationWithoutBackendEdits) {
    MachineRecipeRegistry recipes;
    assets::atomic::compilation::installIncrementI32(recipes, I32);
    ir::MIRBuilder mir;
    auto x=mir.emitValue("novac.i32.const",{O::fromLiteral(41)},I32);
    auto y=mir.emitValue("atomic.i32.increment",{O::fromValue(x)},I32);
    mir.terminate("return",{O::fromValue(y)});
    auto module=mir.finish();
    CHECK(failure([&] { backend::x86_64::selectAndEncode(module, {}); }).find("no compatible recipe")!=std::string::npos);
    backend::X86_64Backend custom{{},recipes};
    const auto caps=custom.capabilities();
    CHECK(std::find(caps->instructions.begin(),caps->instructions.end(),"atomic.i32.increment")!=caps->instructions.end());
    auto image=backend::x86_64::selectAndEncode(module,{},
       [&]{auto r=backend::x86_64::standardRecipes({});r.append(recipes);return r;}(),{});
    CHECK(!image.text.empty());
    // add eax, imm32 from the external Atomic pack, encoded without GCC/as.
    CHECK(std::find(image.text.begin(),image.text.end(),0x05)!=image.text.end());
}

TEST(MachineRecipes, OverrideMatchesOnlyZeroConstantAndChangesEncodedBytes) {
    MachineRecipeRegistry extra;
    assets::atomic::compilation::installZeroConstantOverride(extra,I32);
    auto all=backend::x86_64::standardRecipes({}); all.append(extra);
    const auto zero=single("novac.i32.const",I32,0);
    const auto nonzero=single("novac.i32.const",I32,12);
    auto oldZero=backend::x86_64::selectAndEncode(zero,{});
    auto changedZero=backend::x86_64::selectAndEncode(zero,{},all,{});
    CHECK(changedZero.text.size() == oldZero.text.size()+5);
    CHECK(backend::x86_64::selectAndEncode(nonzero,{}).text ==
          backend::x86_64::selectAndEncode(nonzero,{},all,{}).text);
}

TEST(MachineRecipes, CostAndGoalSelectDifferentCompatibleVariants) {
    MachineRecipeRegistry rules;
    auto fast=makeRule("fast","constant.special",I32);
    fast.cost={1,10};rules.add(fast);
    auto small=makeRule("small","constant.special",I32);
    small.cost={6,4};rules.add(small);
    auto module=single("constant.special",I32,0);
    auto inst=module.blocks[0].instructions[0];
    MachineTarget speed{};speed.goal=OptimizationGoal::Speed;
    MachineTarget size{};size.goal=OptimizationGoal::Size;
    CHECK(rules.select(inst,speed).id == "fast");
    CHECK(rules.select(inst,size).id == "small");
}

TEST(MachineRecipes, FeatureRequirementsFilterAndFallback) {
    MachineRecipeRegistry rules;
    auto generic=makeRule("generic","x.custom",I32);generic.tier=RecipeTier::Standard;
    rules.add(generic);
    auto avx=makeRule("avx","x.custom",I32);avx.requiredCpuFeatures={"avx2"};rules.add(avx);
    auto inst=single("x.custom",I32,0).blocks[0].instructions[0];
    MachineTarget target{};
    CHECK(rules.select(inst,target).id=="generic");
    target.cpuFeatures.insert("avx2");
    CHECK(rules.select(inst,target).id=="avx");
    target.architecture="arm64";
    CHECK(failure([&] { (void)rules.select(inst,target); }).find("no compatible recipe")!=std::string::npos);
}

TEST(MachineRecipes, AmbiguousSelectionAndDuplicateIdsAreRejected) {
    MachineRecipeRegistry rules;
    auto a=makeRule("ownerA","x.custom",I32);rules.add(a);
    CHECK(failure([&] {rules.add(a);}).find("duplicate recipe id")!=std::string::npos);
    rules.add(makeRule("ownerB","x.custom",I32));
    auto module=single("x.custom",I32,0);
    CHECK(failure([&] { (void)rules.select(module.blocks[0].instructions[0],{}); }).find("ambiguous recipes")!=std::string::npos);
    MachineRecipeRegistry all;all.add(a);
    MachineRecipeRegistry pack;pack.add(makeRule("ownerB","x.custom",I32));pack.add(a);
    CHECK(failure([&] { all.append(pack); }).find("duplicate recipe id")!=std::string::npos);
    CHECK(all.recipes().size()==1);
}

TEST(MachineRecipes, TypedPackUsesCanonicalAliasesAndLeavesOtherAssetsUncoupled) {
    assets::types::TypeController types;
    types.definePrimitive("u32").bits(32).unsignedType().commit();
    types.registerAlias("word",assets::types::TypeId{"u32"});
    types.finalize();
    assets::types::IRTypeAdapter adapter{types,"my.language"};
    MachineRecipeRegistry registry;
    auto rule=makeRule("u32.special","novac.i32.const",I32);
    assets::types::compilation::addTypedRecipe(registry,adapter,assets::types::TypeId{"word"},rule);
    auto canonical=adapter.toIR(assets::types::TypeId{"u32"});
    CHECK(registry.recipes()[0].resultType==canonical);
    // u32 can be represented using the backend 32-bit accumulator for this
    // constant-only test; signed arithmetic and ABI are not inferred here.
    backend::x86_64::ValueTypes mapped{canonical,ir::ValueType::Bool};
    auto all=backend::x86_64::standardRecipes(mapped); all.append(registry);
    auto image=backend::x86_64::selectAndEncode(single("novac.i32.const",canonical,12),mapped,all,{});
    CHECK(!image.text.empty());
}

TEST(MachineRecipes, InvalidMachineIROrSSATypeDoesNotSilentlyCompile) {
    MachineRecipeRegistry invalid;
    auto bad=makeRule("bad.bool.add","novac.bool.const",ir::ValueType::Bool);
    bad.emit=[](const ir::Instruction &, MachineBuilder &b){ b.addImmediate(1); };
    invalid.add(bad);
    auto all=backend::x86_64::standardRecipes({});all.append(invalid);
    CHECK(failure([&] {backend::x86_64::selectAndEncode(single("novac.bool.const",ir::ValueType::Bool,1),{},all,{});})
          .find("first instruction must define")!=std::string::npos);
    MachineRecipeRegistry wrong;
    auto arith=makeRule("bad.add","novac.i32.add",I32);
    arith.emit=[](const ir::Instruction &,MachineBuilder &b){b.loadImmediate(5);b.addInput(0);};
    wrong.add(arith);
    auto all2=backend::x86_64::standardRecipes({});all2.append(wrong);
    auto mir=arithmetic("novac.i32.add",1,2);
    // The recipe is well-typed, but can be semantically wrong: contracts and
    // equivalence tests remain necessary and are NOT a verifier guarantee.
    CHECK(!backend::x86_64::selectAndEncode(mir,{},all2,{}).text.empty());
}

TEST(MachineRecipes, RejectsIncompleteRulesBeforeBackendConstruction) {
    MachineRecipeRegistry registry;
    MachineRecipe empty{};
    CHECK(failure([&]{registry.add(empty);}).find("incomplete")!=std::string::npos);
    CHECK(failure([&]{backend::X86_64Backend bad{{}, {}, {"arm64"}};}).find("unsupported target")!=std::string::npos);
}

TEST(MachineRecipes, AtomicCustomOperationExecutesNativelyAndReturns42) {
    MachineRecipeRegistry pack;
    assets::atomic::compilation::installIncrementI32(pack,I32);
    ir::MIRBuilder builder;
    auto fortyOne=builder.emitValue("novac.i32.const",{O::fromLiteral(41)},I32);
    auto result=builder.emitValue("atomic.i32.increment",{O::fromValue(fortyOne)},I32);
    builder.terminate("return",{O::fromValue(result)});
    compilation::CompilationSession session;
    session.artifacts().insert<ir::MIRModule>("mir",builder.finish());
    backend::BackendRegistry registry;
    registry.add("x64",std::make_shared<backend::X86_64Backend>(backend::x86_64::ValueTypes{},pack));
    registry.runChecked("x64",session);
    const auto &elf = session.artifacts().require<std::vector<std::uint8_t>>("x86-64-elf-executable");
    const auto &object = session.artifacts().require<std::vector<std::uint8_t>>("x86-64-elf-object");
    CHECK(elf.size()>64);
    CHECK(object.size()>64);
#if defined(__linux__) && defined(__x86_64__)
    CHECK(runNative(elf)==42);
#endif
}

TEST(MachineRecipes, ZeroOverrideExecutesCorrectlyAndNonzeroFallsBack) {
    MachineRecipeRegistry pack;
    assets::atomic::compilation::installZeroConstantOverride(pack,I32);
    auto complete=backend::x86_64::standardRecipes({}); complete.append(pack);
    auto zero=backend::x86_64::selectAndEncode(single("novac.i32.const",I32,0),{},complete,{});
    auto nonzero=backend::x86_64::selectAndEncode(single("novac.i32.const",I32,17),{},complete,{});
#if defined(__linux__) && defined(__x86_64__)
    CHECK(runNative(backend::x86_64::makeExecutable(zero.text))==0);
    CHECK(runNative(backend::x86_64::makeExecutable(nonzero.text))==17);
#else
    CHECK(!zero.text.empty());
    CHECK(!nonzero.text.empty());
#endif
}

TEST(MachineRecipes, OptionalAtomicPackCompilesASTThroughHIRAndMIRToNativeELF) {
    compilation::ModularCompilationController compiler;
    compiler.features().attach(compilation::CompilationFacet{
        "atomic.integer", "Integer", "atomic.literal.i32",
        [](const ast::Node &node, compilation::FeatureLoweringContext &context) -> std::optional<ir::ValueId> {
            return context.builder().emitValue("novac.i32.const",
                {ir::Operand::fromLiteral(node.integer("value"))},context.semantic().type);
        }
    });
    compiler.features().attach(compilation::CompilationFacet{
        "language.program", "Program", "program.return",
        [](const ast::Node &node, compilation::FeatureLoweringContext &context) -> std::optional<ir::ValueId> {
            auto value=context.requireValue(*node.child("body"));
            context.builder().terminate("return",{ir::Operand::fromValue(value)});
            return {};
        }
    });
    compiler.lowering().mir("novac.i32.const", [](const ir::MIRLoweringInput &in, ir::MIRBuilder &builder,
                                const ir::MIRLoweringContext &, const ir::LoweringRegistry &)
                                -> ir::MIRLoweringResult {
        return builder.emitValue(in.source.op,in.operands,in.source.type);
    });
    assets::atomic::compilation::installIncrementLowering(compiler);
    MachineRecipeRegistry pack;
    assets::atomic::compilation::installIncrementI32(pack,I32);
    compiler.backends().add("x64",std::make_shared<backend::X86_64Backend>(
        backend::x86_64::ValueTypes{},pack));

    auto integer=ast::Node::make("Integer");integer->set("value",41);
    auto increment=ast::Node::make("UnaryExpr");increment->set("operand",integer);
    auto root=ast::Node::make("Program");root->set("body",increment);
    const compilation::SemanticResolver resolve=[](const ast::Node &node)->compilation::ResolvedOperation {
        if (node.kind()=="Integer") return {"atomic.literal.i32",I32};
        if (node.kind()=="UnaryExpr") return {"atomic.increment.i32",I32};
        if (node.kind()=="Program") return {"program.return",ir::ValueType::Void};
        throw std::runtime_error("unknown AST node");
    };
    auto session=compiler.compile(*root,resolve,"x64");
    CHECK(session.artifacts().has("hir"));
    CHECK(session.artifacts().has("mir"));
    const auto &elf=session.artifacts().require<std::vector<std::uint8_t>>("x86-64-elf-executable");
#if defined(__linux__) && defined(__x86_64__)
    CHECK(runNative(elf)==42);
#else
    CHECK(!elf.empty());
#endif
}

TEST(MachineRecipes, LaterOverrideWinsEvenIfEarlierStandardsTie) {
    MachineRecipeRegistry registry;
    auto first=makeRule("first.standard","x.priority",I32);
    auto second=makeRule("second.standard","x.priority",I32);
    first.tier=RecipeTier::Standard;
    second.tier=RecipeTier::Standard;
    auto override=makeRule("final.override","x.priority",I32);
    registry.add(first);
    registry.add(second);
    registry.add(override);
    auto inst=single("x.priority",I32,0).blocks[0].instructions[0];
    CHECK(registry.select(inst,{}).id=="final.override");
    MachineRecipeRegistry shuffled;
    shuffled.add(override); shuffled.add(second); shuffled.add(first);
    CHECK(shuffled.select(inst,{}).id=="final.override");
}

TEST(MachineRecipes, InapplicableTypedOverrideDoesNotHideStandardFallback) {
    MachineRecipeRegistry registry;
    auto fallback=makeRule("fallback","x.typed",I32);
    fallback.tier=RecipeTier::Standard;
    registry.add(fallback);
    auto wrong=makeRule("wrong.typed.override","x.typed",I32);
    wrong.operandTypes=std::vector<ir::TypeRef>{I32};
    registry.add(wrong);
    const auto literal=single("x.typed",I32,9).blocks[0].instructions[0];
    CHECK(registry.select(literal,{}).id=="fallback");
    // A type mismatch also makes an SSA-specific recipe ineligible.
    const auto ssa=arithmetic("x.typed",1,2).blocks[0].instructions[2];
    auto wrongResolver=[](ir::ValueId) -> std::optional<ir::TypeRef> {
        return ir::TypeRef{ir::ValueType::Bool};
    };
    CHECK(registry.select(ssa,{},wrongResolver).id=="fallback");
    auto goodResolver=[](ir::ValueId) -> std::optional<ir::TypeRef> {return I32;};
    // It has 2 operands, so the typed arity-1 override is still ineligible.
    CHECK(registry.select(ssa,{},goodResolver).id=="fallback");
}

TEST(MachineRecipes, OperandPatternsMatchSSAAndLiteralKindsBeforeRanking) {
    MachineRecipeRegistry recipes;
    auto base=makeRule("generic","x.kind",I32);base.tier=RecipeTier::Standard;
    recipes.add(base);
    auto integer=makeRule("int.only","x.kind",I32);
    integer.operandPatterns=std::vector<OperandPattern>{{OperandKind::IntegerLiteral,{}}};
    recipes.add(integer);
    auto inst=single("x.kind",I32,7).blocks[0].instructions[0];
    CHECK(recipes.select(inst,{}).id=="int.only");
    ir::MIRBuilder builder;
    builder.emitValue("x.kind",{O::fromLiteral(true)},I32);
    builder.terminate("return");
    CHECK(recipes.select(builder.finish().blocks[0].instructions[0],{}).id=="generic");
    auto invalid=makeRule("invalid-pattern","x.kind",I32);
    invalid.operandPatterns=std::vector<OperandPattern>{{OperandKind::IntegerLiteral,I32}};
    CHECK(failure([&] {recipes.add(invalid);}).find("invalid typed operand pattern")!=std::string::npos);
}

TEST(MachineRecipes, OperandTypesAreCheckedAgainstActualBackendSSAValues) {
    MachineRecipeRegistry pack;
    auto wrong=makeRule("bad.type.override","novac.i32.add",I32);
    wrong.operandTypes=std::vector<ir::TypeRef>{I32,ir::TypeRef{ir::ValueType::Bool}};
    pack.add(wrong);
    auto defaults=backend::x86_64::standardRecipes({});
    defaults.append(pack);
    const auto baseline=backend::x86_64::selectAndEncode(arithmetic("novac.i32.add",20,22),{});
    const auto selected=backend::x86_64::selectAndEncode(arithmetic("novac.i32.add",20,22),{},defaults,{});
    CHECK(selected.text==baseline.text);
}

TEST(MachineRecipes, RejectsNoncanonicalBooleanMachineValueWithContext) {
    MachineRecipeRegistry pack;
    auto bad=makeRule("bool.invalid", "novac.bool.const", ir::ValueType::Bool);
    bad.emit=[](const ir::Instruction&, MachineBuilder &builder) {builder.loadImmediate(255);};
    pack.add(bad);
    auto defaults=backend::x86_64::standardRecipes({}); defaults.append(pack);
    ir::MIRBuilder builder;
    auto v=builder.emitValue("novac.bool.const",{O::fromLiteral(true)},ir::ValueType::Bool);
    auto t=builder.createBlock("true"), f=builder.createBlock("false");
    builder.terminateWithSuccessors("branch",{O::fromValue(v)},{{t,{}},{f,{}}});
    builder.setCurrentBlock(t); auto yes=builder.emitValue("novac.i32.const",{O::fromLiteral(1)},I32);
    builder.terminate("return",{O::fromValue(yes)});
    builder.setCurrentBlock(f); auto no=builder.emitValue("novac.i32.const",{O::fromLiteral(0)},I32);
    builder.terminate("return",{O::fromValue(no)});
    const auto message=failure([&] { (void)backend::x86_64::selectAndEncode(builder.finish(),{},defaults,{}); });
    CHECK(message.find("noncanonical boolean")!=std::string::npos);
    CHECK(message.find("block 0")!=std::string::npos);
    CHECK(message.find("novac.bool.const")!=std::string::npos);
}

TEST(MachineRecipes, PrefersSpecificOperandShapeRegardlessOfRegistrationOrder) {
    const auto mir=single("custom.shape",I32,0);
    const auto &inst=mir.blocks[0].instructions[0];
    auto generic=makeRule("generic-shape","custom.shape",I32);
    generic.operandPatterns=std::vector<OperandPattern>{{OperandKind::Any,{}}};
    auto specialized=makeRule("integer-shape","custom.shape",I32);
    specialized.operandPatterns=std::vector<OperandPattern>{{OperandKind::IntegerLiteral,{}}};
    for (int n=0;n<2;++n) {
        MachineRecipeRegistry registry;
        if(n==0) {registry.add(generic);registry.add(specialized);}
        else {registry.add(specialized);registry.add(generic);}
        CHECK(registry.select(inst,{}).id=="integer-shape");
    }
    auto boolean=single("custom.shape",I32,0).blocks[0].instructions[0];
    boolean.operands={O::fromLiteral(true)};
    MachineRecipeRegistry registry;registry.add(generic);registry.add(specialized);
    CHECK(registry.select(boolean,{}).id=="generic-shape");
}
TEST(MachineRecipes, ExactSSATypesWinOverUntypedSSAShape) {
    auto mir=arithmetic("custom.ssa",1,2);
    const auto &inst=mir.blocks[0].instructions[2];
    auto generic=makeRule("untyped-ssa","custom.ssa",I32);
    generic.operandPatterns=std::vector<OperandPattern>{{OperandKind::SSAValue,{}},{OperandKind::SSAValue,{}}};
    auto typed=makeRule("typed-ssa","custom.ssa",I32);
    typed.operandTypes=std::vector<ir::TypeRef>{I32,I32};
    MachineRecipeRegistry registry;registry.add(generic);registry.add(typed);
    CHECK(registry.select(inst,{},[&](ir::ValueId){return std::optional<ir::TypeRef>{I32};}).id=="typed-ssa");
    CHECK(registry.select(inst,{}).id=="untyped-ssa");
}
TEST(MachineRecipes, EquallyPreciseShapesStillReportAmbiguity) {
    MachineRecipeRegistry recipes;
    auto first=makeRule("left-specific","custom.two",I32);
    first.operandPatterns=std::vector<OperandPattern>{{OperandKind::SSAValue,{}},{OperandKind::Any,{}}};
    auto second=makeRule("right-specific","custom.two",I32);
    second.operandPatterns=std::vector<OperandPattern>{{OperandKind::Any,{}},{OperandKind::SSAValue,{}}};
    recipes.add(first);recipes.add(second);
    auto mir=arithmetic("custom.two",1,2);
    CHECK(failure([&]{ (void)recipes.select(mir.blocks[0].instructions[2],{}); }).find("ambiguous recipes")!=std::string::npos);
}
TEST(MachineRecipes, RejectsContradictorySSAAndLiteralMatchers) {
    MachineRecipeRegistry recipes;
    auto wrong=makeRule("bad-pattern","custom.match",I32);
    wrong.operandTypes=std::vector<ir::TypeRef>{I32};
    wrong.operandPatterns=std::vector<OperandPattern>{{OperandKind::IntegerLiteral,{}}};
    CHECK(failure([&]{ recipes.add(wrong); }).find("cannot match literal")!=std::string::npos);
}

TEST(MachineRecipes, CompilationOptionsChangeMachineOutputPerSessionWithoutMutatingProvider) {
    MachineRecipeRegistry variants;
    auto speed=makeRule("speed.constant", "novac.i32.const", I32);
    speed.tier=RecipeTier::Override;
    speed.cost={1,10};
    speed.emit=[](const ir::Instruction &,MachineBuilder &b){ b.loadImmediate(42); };
    variants.add(speed);
    auto size=makeRule("small.constant", "novac.i32.const", I32);
    size.tier=RecipeTier::Override;
    size.cost={10,1};
    size.emit=[](const ir::Instruction &,MachineBuilder &b){ b.loadImmediate(24); };
    variants.add(size);
    auto provider=backend::makeX86_64LinuxTarget({},variants);
    const auto mir=single("novac.i32.const",I32,0);
    auto build=[&](OptimizationGoal optimization) {
        compilation::CompilationSession session;
        session.artifacts().insert<ir::MIRModule>("mir",mir);
        backend::NativeBackend backend{provider, compilation::CompilationOptions{optimization}};
        backend.run(session);
        return session.artifacts().require<std::vector<std::uint8_t>>("x86-64-elf-executable");
    };
    const auto fast=build(OptimizationGoal::Speed);
    const auto small=build(OptimizationGoal::Size);
    CHECK(fast != small);
#if defined(__linux__) && defined(__x86_64__)
    CHECK(runNative(fast)==42);
    CHECK(runNative(small)==24);
#endif
    CHECK(build(OptimizationGoal::Speed)==fast);
}

TEST(MachineRecipe, StatefulPredicateEvaluatedOncePerCompilation) {
    using namespace novac;
    using namespace novac::compilation::machine;
    using novac::ir::Operand;
    int calls=0;
    MachineRecipeRegistry custom;
    MachineRecipe customized;
    customized.id="custom.only-once";
    customized.owner="external.asset";
    customized.operation="novac.i32.const";
    customized.resultType=ir::TypeRef{"i32","novac.prototype"};
    customized.tier=RecipeTier::Override;
    customized.matches=[&](const ir::Instruction &){ return ++calls==1; };
    customized.emit=[](const ir::Instruction &,MachineBuilder &b){ b.loadSignedImmediate(42); };
    custom.add(std::move(customized));
    backend::X86_64Backend target{{},custom};
    ir::MIRBuilder builder;
    const auto v=builder.emitValue("novac.i32.const",{Operand::fromLiteral(42)},
                                    ir::TypeRef{"i32","novac.prototype"});
    builder.terminate("return",{Operand::fromValue(v)});
    compilation::CompilationSession session;
    session.artifacts().insert<ir::MIRModule>("mir",builder.finish());
    target.run(session);
    CHECK(calls==1);
    CHECK(session.artifacts().has("x86-64-elf-executable"));
}

TEST(MachineRecipe, CheckedBackendUsesSamePerGenerationSelectionPlan) {
    using namespace novac;
    using namespace novac::compilation::machine;
    int checks=0;
    MachineRecipeRegistry customization;
    MachineRecipe rule;
    rule.id="checked.one-shot";
    rule.owner="external.asset";
    rule.operation="novac.i32.const";
    rule.resultType=ir::TypeRef{"i32","novac.prototype"};
    rule.tier=RecipeTier::Override;
    rule.matches=[&](const ir::Instruction &) { ++checks; return checks==1; };
    rule.emit=[](const ir::Instruction &,MachineBuilder &b) { b.loadSignedImmediate(42); };
    customization.add(std::move(rule));
    backend::BackendRegistry providers;
    providers.add("native",std::make_shared<backend::X86_64Backend>(
        backend::x86_64::ValueTypes{},customization));
    ir::MIRBuilder builder;
    auto value=builder.emitValue("novac.i32.const",{ir::Operand::fromLiteral(42)},
                                 ir::TypeRef{"i32","novac.prototype"});
    builder.terminate("return",{ir::Operand::fromValue(value)});
    compilation::CompilationSession session;
    session.artifacts().insert<ir::MIRModule>("mir",builder.finish());
    providers.runChecked("native",session);
    CHECK(checks==1);
    CHECK(session.artifacts().has("x86-64-elf-executable"));
}
