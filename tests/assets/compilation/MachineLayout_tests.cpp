#include "../../tester.hpp"
#include "novac/assets/compilation/backend/X86_64Backend.hpp"
#include "novac/assets/compilation/backend/x86_64/Codegen.hpp"
#include "novac/assets/compilation/backend/x86_64/StandardRecipes.hpp"
#include "novac/assets/compilation/machine/MachineLayout.hpp"
#include "novac/assets/types/compilation/TypeMachineRecipes.hpp"

#include <cstdint>
#include <limits>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>
#if defined(__linux__) && defined(__x86_64__)
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace {
using namespace novac;
using namespace novac::compilation::machine;
using O=ir::Operand;
const ir::TypeRef U8{"u8","m2.tests"}, U16{"u16","m2.tests"};
const ir::TypeRef I8{"i8","m2.tests"}, U64{"u64","m2.tests"};
const ir::TypeRef I32{"i32","novac.prototype"};
const ir::TypeRef Bool{ir::ValueType::Bool};

template<typename F> std::string errorText(F &&f) {
    try { f(); } catch (const std::runtime_error &e) { return e.what(); }
    return {};
}
#if defined(__linux__) && defined(__x86_64__)
int runMachine(const std::vector<std::uint8_t> &bytes) {
    char path[]="/tmp/novac-m2-machine-XXXXXX";
    const int fd=mkstemp(path);
    if(fd<0) throw std::runtime_error("temp file failed");
    if(fchmod(fd,0700)!=0) {close(fd);unlink(path);throw std::runtime_error("chmod failed");}
    std::size_t offset=0;
    while(offset<bytes.size()) {
        const auto n=write(fd,bytes.data()+offset,bytes.size()-offset);
        if(n<=0) {close(fd);unlink(path);throw std::runtime_error("write failed");}
        offset+=static_cast<std::size_t>(n);
    }
    close(fd);
    const pid_t child=fork();
    if(child<0) {unlink(path);throw std::runtime_error("fork failed");}
    if(child==0) {execl(path,"novac-m2",static_cast<char *>(nullptr)); _exit(252);}
    int status=0;
    const pid_t waited=waitpid(child,&status,0);
    unlink(path);
    if(waited!=child || !WIFEXITED(status)) throw std::runtime_error("machine executable failed");
    return WEXITSTATUS(status);
}
#endif
struct Support {
    backend::x86_64::ValueTypes types{};
    MachineRecipeRegistry recipes{backend::x86_64::standardRecipes(types)};
    Support() {
        types.layouts.add(U8,{8,ScalarKind::UnsignedInteger,1});
        types.layouts.add(U16,{16,ScalarKind::UnsignedInteger,2});
        types.layouts.add(I8,{8,ScalarKind::SignedInteger,1});
        types.layouts.add(U64,{64,ScalarKind::UnsignedInteger,8});
        backend::x86_64::installScalarRecipes(recipes,U8,{8,ScalarKind::UnsignedInteger,1},"u8");
        backend::x86_64::installScalarRecipes(recipes,U16,{16,ScalarKind::UnsignedInteger,2},"u16");
        backend::x86_64::installScalarRecipes(recipes,I8,{8,ScalarKind::SignedInteger,1},"i8");
        backend::x86_64::installScalarRecipes(recipes,U64,{64,ScalarKind::UnsignedInteger,8},"u64");
    }
    backend::x86_64::MachineImage compile(const ir::MIRModule &module) const {
        return backend::x86_64::selectAndEncode(module,types,recipes,{});
    }
    std::vector<std::uint8_t> executable(const ir::MIRModule &module) const {
        MachineRecipeRegistry custom{};
        for (const auto &r : recipes.recipes()) {
            if (!r.id.starts_with("x64.standard.")) custom.add(r);
        }
        backend::X86_64Backend backend{types,custom};
        compilation::CompilationSession session{};
        session.artifacts().insert<ir::MIRModule>("mir",module);
        backend.run(session);
        return session.artifacts().require<std::vector<std::uint8_t>>("x86-64-elf-executable");
    }
};
ir::ValueId constant(ir::MIRBuilder &b, std::string name, ir::TypeRef type, int value) {
    return b.emitValue("novac."+name+".const",{O::fromLiteral(value)},type);
}
}

TEST(MachineLayout, ValidatesIdentityWidthSignednessAndAlignment) {
    MachineLayoutRegistry registry;
    registry.add(U16,{16,ScalarKind::UnsignedInteger,2});
    CHECK(registry.require(U16).bytes()==2);
    CHECK(errorText([&]{registry.add(U16,{16,ScalarKind::UnsignedInteger,2});}).find("duplicate")!=std::string::npos);
    CHECK(errorText([&]{(void)registry.require(U8);}).find("no machine representation")!=std::string::npos);
    CHECK(!errorText([&]{MachineLayout{24,ScalarKind::UnsignedInteger,1}.validate();}).empty());
    CHECK(!errorText([&]{MachineLayout{16,ScalarKind::Boolean,2}.validate();}).empty());
    CHECK(!errorText([&]{MachineLayout{16,ScalarKind::UnsignedInteger,3}.validate();}).empty());
}

TEST(MachineLayout, TypesAssetCanonicalAliasesProduceSamePhysicalType) {
    assets::types::TypeController types;
    types.definePrimitive("u16").bits(16).alignmentBytes(2).unsignedType().commit();
    types.registerAlias("word",assets::types::TypeId{"u16"});
    types.finalize();
    assets::types::IRTypeAdapter adapter{types,"m2.language"};
    MachineLayoutRegistry registry;
    assets::types::compilation::addPrimitiveLayout(registry,adapter,types,assets::types::TypeId{"word"});
    CHECK(adapter.toIR(assets::types::TypeId{"u16"})==adapter.toIR(assets::types::TypeId{"word"}));
    CHECK(registry.require(adapter.toIR(assets::types::TypeId{"u16"})).bits==16);
}

TEST(MachineLayout, NativeUnsigned16AdditionReturns42) {
    Support s;
    ir::MIRBuilder b;
    auto a=constant(b,"u16",U16,20), c=constant(b,"u16",U16,22);
    auto sum=b.emitValue("novac.u16.add",{O::fromValue(a),O::fromValue(c)},U16);
    b.terminate("return",{O::fromValue(sum)});
#if defined(__linux__) && defined(__x86_64__)
    CHECK(runMachine(s.executable(b.finish()))==42);
#else
    CHECK(!s.compile(b.finish()).text.empty());
#endif
}

TEST(MachineLayout, Unsigned8TruncatesAt255AndSigned8ComparesCorrectly) {
    Support s;
    for(const auto &[x,y,expected]:std::vector<std::tuple<int,int,int>>{{255,1,0},{255,0,255},{1,42,43}}) {
        ir::MIRBuilder b;
        const auto a=constant(b,"u8",U8,x),z=constant(b,"u8",U8,y);
        const auto v=b.emitValue("novac.u8.add",{O::fromValue(a),O::fromValue(z)},U8);
        b.terminate("return",{O::fromValue(v)});
#if defined(__linux__) && defined(__x86_64__)
        CHECK(runMachine(s.executable(b.finish()))==expected);
#else
        CHECK(!s.compile(b.finish()).text.empty());
#endif
    }
    ir::MIRBuilder b;
    auto a=constant(b,"i8",I8,-1),z=constant(b,"i8",I8,0);
    auto isLess=b.emitValue("novac.i8.lt",{O::fromValue(a),O::fromValue(z)},Bool);
    auto t=b.createBlock("true"),f=b.createBlock("false");
    auto answer=constant(b,"u8",U8,42),wrong=constant(b,"u8",U8,1);
    b.terminateWithSuccessors("branch",{O::fromValue(isLess)},{{t,{}},{f,{}}});
    b.setCurrentBlock(t);b.terminate("return",{O::fromValue(answer)});
    b.setCurrentBlock(f);b.terminate("return",{O::fromValue(wrong)});
#if defined(__linux__) && defined(__x86_64__)
    CHECK(runMachine(s.executable(b.finish()))==42);
#else
    CHECK(!s.compile(b.finish()).text.empty());
#endif
}

TEST(MachineLayout, RejectsOutOfRangeConstantsAndUnknownLayouts) {
    Support s;
    for(const auto &[name,type,value]: std::vector<std::tuple<std::string,ir::TypeRef,int>>{
        {"u8",U8,256},{"u8",U8,-1},{"i8",I8,128},{"i8",I8,-129},{"u16",U16,65536}}) {
        ir::MIRBuilder b;
        auto a=constant(b,name,type,value);b.terminate("return",{O::fromValue(a)});
        CHECK(errorText([&]{s.compile(b.finish());}).find("out of range")!=std::string::npos);
    }
    ir::MIRBuilder b;
    auto a=b.emitValue("unknown.const",{O::fromLiteral(2)},ir::TypeRef{"u24","unmapped"});
    b.terminate("return",{O::fromValue(a)});
    CHECK(errorText([&]{s.compile(b.finish());}).find("unsupported SSA type")!=std::string::npos);
}

TEST(MachineLayout, ExplicitConversionWidensAndNarrows) {
    Support s;
    MachineRecipe w{};
    w.id="convert.u8.to.u16";w.operation="convert.u8.u16";w.resultType=U16;
    w.operandTypes=std::vector<ir::TypeRef>{U8};
    w.emit=[](const ir::Instruction &, MachineBuilder &b){b.convertInput(0);};
    s.recipes.add(w);
    w.id="convert.u16.to.u8";w.operation="convert.u16.u8";w.resultType=U8;
    w.operandTypes=std::vector<ir::TypeRef>{U16};
    s.recipes.add(w);
    ir::MIRBuilder b;
    auto v=constant(b,"u8",U8,42);
    auto widened=b.emitValue("convert.u8.u16",{O::fromValue(v)},U16);
    b.terminate("return",{O::fromValue(widened)});
#if defined(__linux__) && defined(__x86_64__)
    CHECK(runMachine(s.executable(b.finish()))==42);
#else
    CHECK(!s.compile(b.finish()).text.empty());
#endif
    ir::MIRBuilder narrow;
    const auto wide=constant(narrow,"u16",U16,256);
    auto result=narrow.emitValue("convert.u16.u8",{O::fromValue(wide)},U8);
    narrow.terminate("return",{O::fromValue(result)});
#if defined(__linux__) && defined(__x86_64__)
    CHECK(runMachine(s.executable(narrow.finish()))==0);
#endif
}

TEST(MachineLayout, Native64BitOpsUseRexWAndUnsignedComparison) {
    Support s;
    MachineRecipe huge{};
    huge.id="huge.const";huge.operation="novac.u64.huge";huge.resultType=U64;
    huge.operandPatterns=std::vector<OperandPattern>{};
    huge.emit=[](const ir::Instruction &,MachineBuilder &b){b.loadImmediate64(std::uint64_t{1}<<40);};
    s.recipes.add(huge);
    ir::MIRBuilder b;
    const auto a=b.emitValue("novac.u64.huge",{},U64);
    const auto z=constant(b,"u64",U64,42);
    const auto lt=b.emitValue("novac.u64.lt",{O::fromValue(z),O::fromValue(a)},Bool);
    auto yes=b.createBlock("yes"),no=b.createBlock("no");
    b.terminateWithSuccessors("branch",{O::fromValue(lt)},{{yes,{}},{no,{}}});
    b.setCurrentBlock(yes);b.terminate("return",{O::fromValue(z)});
    b.setCurrentBlock(no);b.terminate("return",{O::fromValue(a)});
    const auto mod=b.finish();
    auto image=s.compile(mod);
    CHECK(std::find(image.text.begin(),image.text.end(),std::uint8_t{0x48})!=image.text.end());
#if defined(__linux__) && defined(__x86_64__)
    CHECK(runMachine(s.executable(mod))==42);
#endif
}

TEST(MachineLayout, EffectOnlyNopExecutesWithoutFabricatedSSAResult) {
    Support support;
    ir::MIRBuilder b;
    b.emit("novac.machine.nop");
    const auto value=constant(b,"u16",U16,42);
    b.terminate("return",{O::fromValue(value)});
#if defined(__linux__) && defined(__x86_64__)
    CHECK(runMachine(support.executable(b.finish()))==42);
#else
    CHECK(!support.compile(b.finish()).text.empty());
#endif
}

TEST(MachineLayout, CustomEffectRecipeAllowsTrapButNotAValue) {
    Support support;
    MachineRecipe effect{};
    effect.id="custom.effect.trap";
    effect.owner="external.custom.asset";
    effect.operation="custom.trap";
    effect.operandPatterns=std::vector<OperandPattern>{};
    effect.emit=[](const ir::Instruction &,MachineBuilder &builder){builder.trap();};
    support.recipes.add(effect);
    ir::MIRBuilder b;
    b.emit("custom.trap");
    auto result=constant(b,"u16",U16,42);
    b.terminate("return",{O::fromValue(result)});
    auto machine=support.compile(b.finish()).text;
    const std::vector<std::uint8_t> opcode{0x0F,0x0B};
    CHECK(std::search(machine.begin(),machine.end(),opcode.begin(),opcode.end()) != machine.end());
}

TEST(MachineLayout, FreezesRecipeRegistryAndPreflightsTypedOperations) {
    MachineRecipeRegistry registry;
    MachineRecipe recipe{};
    recipe.id="custom.only";
    recipe.owner="custom.asset";
    recipe.operation="only.u16";
    recipe.resultType=U16;
    recipe.operandTypes=std::vector<ir::TypeRef>{U16};
    recipe.emit=[](const ir::Instruction &,MachineBuilder &builder){builder.loadInput(0);};
    registry.add(recipe);
    registry.freeze();
    CHECK(registry.frozen());
    CHECK(errorText([&]{registry.add(recipe);}).find("frozen")!=std::string::npos);
    CHECK(errorText([&]{registry.append(MachineRecipeRegistry{});}).find("frozen")!=std::string::npos);

    backend::x86_64::ValueTypes configured{};
    configured.layouts.add(U16,{16,ScalarKind::UnsignedInteger,2});
    backend::X86_64Backend backend{configured,[&]{MachineRecipeRegistry custom;custom.add(recipe);return custom;}()};
    ir::MIRBuilder b;
    auto x=b.emitValue("novac.i32.const",{O::fromLiteral(42)},I32);
    b.emitValue("only.u16",{O::fromValue(x)},U16);
    b.terminate("return",{O::fromValue(x)});
    // Structural preflight does not call user predicates. Recipe selection and
    // verification are performed once inside the per-generation plan.
    auto program = b.finish();
    CHECK(errorText([&]{backend.preflight(program);}).empty());
    compilation::CompilationSession session;
    session.artifacts().insert<ir::MIRModule>("mir",program);
    CHECK(errorText([&]{backend.run(session);}).find("no compatible recipe")!=std::string::npos);
}

TEST(MachineLayout, LanguageWordAliasU16CompilesWithTypesAdapter) {
    assets::types::TypeController language;
    language.declareType("u16");
    language.definePrimitive("u8").bits(8).alignmentBytes(1).unsignedType()
        .conversionTo(assets::types::TypeId{"u16"},assets::types::ConversionKind::Implicit,1).commit();
    language.definePrimitive("u16").bits(16).alignmentBytes(2).unsignedType().commit();
    language.registerAlias("word",assets::types::TypeId{"u16"});
    language.finalize();
    assets::types::IRTypeAdapter adapter{language,"custom.language"};
    backend::x86_64::ValueTypes config{};
    assets::types::compilation::addPrimitiveLayout(config.layouts,adapter,language,
        assets::types::TypeId{"u16"});
    const auto word=adapter.toIR(assets::types::TypeId{"word"});
    MachineRecipeRegistry pack;
    backend::x86_64::installScalarRecipes(pack,word,{16,ScalarKind::UnsignedInteger,2},"u16");
    backend::X86_64Backend native{config,pack};
    ir::MIRBuilder b;
    const auto twenty=constant(b,"u16",word,20),twentyTwo=constant(b,"u16",word,22);
    auto answer=b.emitValue("novac.u16.add",{O::fromValue(twenty),O::fromValue(twentyTwo)},word);
    b.terminate("return",{O::fromValue(answer)});
    compilation::CompilationSession session{};
    session.artifacts().insert<ir::MIRModule>("mir",b.finish());
    native.run(session);
#if defined(__linux__) && defined(__x86_64__)
    CHECK(runMachine(session.artifacts().require<std::vector<std::uint8_t>>("x86-64-elf-executable"))==42);
#else
    CHECK(session.artifacts().has("x86-64-elf-executable"));
#endif
}

TEST(MachineLayout, Signed8ToUnsigned64CastSignExtendsBeforeUnsignedComparison) {
    Support support;
    MachineRecipe cast{};
    cast.id="explicit.int.cast";cast.operation="explicit.int.cast";
    cast.resultType=U64;cast.operandTypes=std::vector<ir::TypeRef>{I8};
    cast.emit=[](const ir::Instruction &,MachineBuilder &b){b.convertInput(0);};
    support.recipes.add(cast);
    MachineRecipe maximum{};
    maximum.id="custom.u64.max";maximum.operation="custom.u64.max";
    maximum.resultType=U64;maximum.operandPatterns=std::vector<OperandPattern>{};
    maximum.emit=[](const ir::Instruction &,MachineBuilder &b){b.loadImmediate64(~std::uint64_t{0});};
    support.recipes.add(maximum);
    ir::MIRBuilder b;
    const auto minusOne=constant(b,"i8",I8,-1);
    const auto extended=b.emitValue("explicit.int.cast",{O::fromValue(minusOne)},U64);
    const auto max=b.emitValue("custom.u64.max",{},U64);
    const auto smaller=b.emitValue("novac.u64.lt",{O::fromValue(extended),O::fromValue(max)},Bool);
    const auto yes=b.createBlock("bad"),no=b.createBlock("good");
    const auto wrong=constant(b,"u8",U8,7),right=constant(b,"u8",U8,42);
    b.terminateWithSuccessors("branch",{O::fromValue(smaller)},{{yes,{}},{no,{}}});
    b.setCurrentBlock(yes);b.terminate("return",{O::fromValue(wrong)});
    b.setCurrentBlock(no);b.terminate("return",{O::fromValue(right)});
#if defined(__linux__) && defined(__x86_64__)
    CHECK(runMachine(support.executable(b.finish()))==42);
#else
    CHECK(!support.compile(b.finish()).text.empty());
#endif
}

TEST(MachineLayout, CustomLanguageBooleanTypeCanBeUsedByScalarComparisonRecipes) {
    const ir::TypeRef nativeBool{"truth","custom.lang"};
    backend::x86_64::ValueTypes config{};
    config.boolean=nativeBool;
    config.layouts.add(U16,{16,ScalarKind::UnsignedInteger,2});
    MachineRecipeRegistry pack;
    backend::x86_64::installScalarRecipes(pack,U16,{16,ScalarKind::UnsignedInteger,2},"u16",nativeBool);
    ir::MIRBuilder b;
    const auto low=constant(b,"u16",U16,20),high=constant(b,"u16",U16,22);
    const auto condition=b.emitValue("novac.u16.lt",{O::fromValue(low),O::fromValue(high)},nativeBool);
    const auto yes=b.createBlock("yes"),no=b.createBlock("no");
    const auto good=constant(b,"u16",U16,42),bad=constant(b,"u16",U16,1);
    b.terminateWithSuccessors("branch",{O::fromValue(condition)},{{yes,{}},{no,{}}});
    b.setCurrentBlock(yes);b.terminate("return",{O::fromValue(good)});
    b.setCurrentBlock(no);b.terminate("return",{O::fromValue(bad)});
    backend::X86_64Backend backend{config,pack};
    compilation::CompilationSession session;
    session.artifacts().insert<ir::MIRModule>("mir",b.finish({.branchBooleanType=nativeBool}));
    backend.run(session);
#if defined(__linux__) && defined(__x86_64__)
    CHECK(runMachine(session.artifacts().require<std::vector<std::uint8_t>>("x86-64-elf-executable"))==42);
#else
    CHECK(session.artifacts().has("x86-64-elf-executable"));
#endif
}

TEST(MachineLayout, CustomRecipeImmediateRangeIsIndependentOfOperationName) {
    Support s;
    auto install=[&](const std::string &op, ir::TypeRef type,
                     std::function<void(const ir::Instruction&,MachineBuilder&)> emit) {
        MachineRecipe recipe;
        recipe.id="custom."+op; recipe.owner="external.asset";
        recipe.operation=op; recipe.resultType=type;
        recipe.tier=RecipeTier::Override;
        recipe.emit=std::move(emit);
        s.recipes.add(std::move(recipe));
    };
    install("u8.make",U8,[](const ir::Instruction&, MachineBuilder &builder){builder.loadIntegerLiteral();});
    install("i8.make",I8,[](const ir::Instruction&, MachineBuilder &builder){builder.loadIntegerLiteral();});
    install("u16.make",U16,[](const ir::Instruction&, MachineBuilder &builder){builder.loadIntegerLiteral();});
    for (const auto &[op, type, value]: std::vector<std::tuple<std::string,ir::TypeRef,int>> {
        {"u8.make",U8,256},{"u8.make",U8,-1},{"i8.make",I8,128},
        {"i8.make",I8,-129},{"u16.make",U16,-1}}) {
        ir::MIRBuilder builder;
        const auto v=builder.emitValue(op,{O::fromLiteral(value)},type);
        builder.terminate("return",{O::fromValue(v)});
        CHECK(errorText([&]{s.compile(builder.finish());}).find("out of range")!=std::string::npos);
    }
    ir::MIRBuilder valid;
    const auto v=valid.emitValue("u8.make",{O::fromLiteral(42)},U8);
    valid.terminate("return",{O::fromValue(v)});
#if defined(__linux__) && defined(__x86_64__)
    CHECK(runMachine(s.executable(valid.finish()))==42);
#else
    CHECK(!s.compile(valid.finish()).text.empty());
#endif
}

TEST(MachineLayout, ExplicitSignedImmediateRejectsUnsignedAndUnsignedRejectsSignedOverflow) {
    Support s;
    MachineRecipe signedRecipe;
    signedRecipe.id="negative64";
    signedRecipe.operation="u64.custom";
    signedRecipe.resultType=U64;
    signedRecipe.emit=[](const ir::Instruction&, MachineBuilder& builder) {
        builder.loadSignedImmediate(-1);
    };
    s.recipes.add(signedRecipe);
    ir::MIRBuilder builder;
    const auto v=builder.emitValue("u64.custom",{},U64);
    builder.terminate("return",{O::fromValue(v)});
    CHECK(errorText([&]{s.compile(builder.finish());}).find("out of range")!=std::string::npos);
}

TEST(MachineLayout, SixtyFourBitLiteralBoundariesKeepSignedness) {
    Support s;
    const ir::TypeRef I64{"i64","m2.tests"};
    s.types.layouts.add(I64,{64,ScalarKind::SignedInteger,8});
    backend::x86_64::installScalarRecipes(s.recipes,I64,{64,ScalarKind::SignedInteger,8},"i64");
    auto makeLiteral=[](ir::MIRBuilder &builder, const std::string& op,
                        ir::TypeRef type, ir::Literal value) {
        return builder.emitValue(op,{O::fromLiteral(std::move(value))},type);
    };
    ir::MIRBuilder signedCase;
    auto smallest=makeLiteral(signedCase,"novac.i64.const",I64,
                             ir::Literal::signedInteger64(std::numeric_limits<std::int64_t>::min()));
    auto zero=makeLiteral(signedCase,"novac.i64.const",I64,ir::Literal::signedInteger64(0));
    auto compare=signedCase.emitValue("novac.i64.lt",{O::fromValue(smallest),O::fromValue(zero)},Bool);
    auto yes=signedCase.createBlock("yes"), no=signedCase.createBlock("no");
    auto a=constant(signedCase,"u8",U8,42),b=constant(signedCase,"u8",U8,24);
    signedCase.terminateWithSuccessors("branch",{O::fromValue(compare)},{{yes,{}},{no,{}}});
    signedCase.setCurrentBlock(yes);signedCase.terminate("return",{O::fromValue(a)});
    signedCase.setCurrentBlock(no);signedCase.terminate("return",{O::fromValue(b)});
#if defined(__linux__) && defined(__x86_64__)
    CHECK(runMachine(s.executable(signedCase.finish()))==42);
#else
    CHECK(!s.compile(signedCase.finish()).text.empty());
#endif
    ir::MIRBuilder unsignedCase;
    auto maximum=makeLiteral(unsignedCase,"novac.u64.const",U64,
                             ir::Literal::unsignedInteger64(std::numeric_limits<std::uint64_t>::max()));
    auto one=makeLiteral(unsignedCase,"novac.u64.const",U64,ir::Literal::unsignedInteger64(1));
    auto less=unsignedCase.emitValue("novac.u64.lt",{O::fromValue(maximum),O::fromValue(one)},Bool);
    auto trueBlock=unsignedCase.createBlock("incorrect"),falseBlock=unsignedCase.createBlock("correct");
    auto wrong=constant(unsignedCase,"u8",U8,24),correct=constant(unsignedCase,"u8",U8,42);
    unsignedCase.terminateWithSuccessors("branch",{O::fromValue(less)},{{trueBlock,{}},{falseBlock,{}}});
    unsignedCase.setCurrentBlock(trueBlock);unsignedCase.terminate("return",{O::fromValue(wrong)});
    unsignedCase.setCurrentBlock(falseBlock);unsignedCase.terminate("return",{O::fromValue(correct)});
#if defined(__linux__) && defined(__x86_64__)
    CHECK(runMachine(s.executable(unsignedCase.finish()))==42);
#else
    CHECK(!s.compile(unsignedCase.finish()).text.empty());
#endif
    ir::MIRBuilder bad;
    auto badValue=makeLiteral(bad,"novac.u64.const",U64,ir::Literal::signedInteger64(-1));
    bad.terminate("return",{O::fromValue(badValue)});
    CHECK(errorText([&]{s.compile(bad.finish());}).find("out of range")!=std::string::npos);
}
