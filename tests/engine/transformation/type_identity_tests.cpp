#include "../../tester.hpp"
#include "novac/engine/transformation/IR.hpp"
#include "novac/engine/transformation/GenericIR.hpp"
#include <string>
#include <stdexcept>
#include <optional>

using namespace novac;
namespace {
template <class F> bool rejects(F &&fn) { try { fn(); } catch (const std::runtime_error &) { return true; } return false; }
}

TEST(TypeIdentity, ScopeIsPartOfTypeIdentity) {
    ir::TypeSpace a{"language.alpha.module1"};
    ir::TypeSpace b{"language.beta.module1"};
    auto ta = a.intern("Player"), tb = b.intern("Player");
    CHECK(ta != tb);
    CHECK(a.contains(ta)); CHECK(!a.contains(tb)); CHECK(b.contains(tb));
    CHECK(a.intern("Player") == ta);
    CHECK((ir::TypeRef{"i8", "lang.x"} != ir::TypeRef{"u64", "lang.x"}));
    CHECK((ir::TypeRef{"Player", "lang.x"} != ir::TypeRef{"Player", "lang.y"}));
}

TEST(TypeIdentity, BuiltinsAndUnknownAreDistinctFromCustomAndDynamic) {
    ir::TypeRef integer{ir::ValueType::Int}, unknown{ir::ValueType::Unknown};
    CHECK(integer == ir::ValueType::Int);
    CHECK((integer != ir::TypeRef{"int", "my.lang"}));
    CHECK(unknown.unresolved()); CHECK(unknown.valid());
    CHECK((!ir::TypeRef{"dynamic", "novac.builtin"}.unresolved()));
    CHECK(!ir::TypeRef{}.valid());
    CHECK(rejects([] { ir::TypeSpace invalid{"novac.builtin"}; }));
}

TEST(TypeIdentity, StableQualifiedSerialization) {
    ir::TypeRef source{"My:Type\n\u03bb", "example:module"};
    CHECK(ir::TypeRef::deserialize(source.serialize()) == source);
    CHECK(ir::TypeRef::deserialize(ir::TypeRef{ir::ValueType::Int}.serialize()) == ir::ValueType::Int);
    CHECK(rejects([] { (void)ir::TypeRef::deserialize("9999999999999999999999999999:a"); }));
    CHECK(rejects([] { (void)ir::TypeRef::deserialize("1:a1:bextra"); }));
    CHECK(rejects([] { (void)ir::TypeRef::deserialize("0:0:"); }));
}

TEST(TypeIdentity, CrossSpaceImportsAreExplicitAndChecked) {
    ir::TypeSpace source{"one"}, dest{"two"};
    ir::TypeImportMap mappings;
    auto from=source.intern("Actor"), to=dest.intern("Actor");
    CHECK(rejects([&] { (void)mappings.resolve(from); }));
    mappings.add(from, to);
    CHECK(mappings.resolve(from) == to);
    CHECK(rejects([&] { mappings.add(from, to); }));
    CHECK((rejects([&] { mappings.add(ir::TypeRef{}, to); })));
}

TEST(TypeIdentity, TypedSSARejectsIncorrectEdgeIdentity) {
    ir::TypeSpace space{"ssa.example"};
    const auto i8=space.intern("i8"), u64=space.intern("u64");
    ir::HIRBuilder builder;
    auto join = builder.createBlock("join");
    builder.addBlockArgument(join,u64);
    auto val=builder.emitValue("const",{},i8);
    builder.terminateWithSuccessors("jump",{},{{join,{ir::Operand::fromValue(val)}}});
    builder.setCurrentBlock(join);
    builder.terminate("return");
    CHECK(rejects([&]{ (void)builder.finish(); }));
}

TEST(TypeIdentity, TypedSSAAcceptsExactIdentityAndStrictSpace) {
    ir::TypeSpace space{"ssa.example"};
    auto i8=space.intern("i8");
    ir::HIRBuilder builder;
    auto join=builder.createBlock("join");
    auto param=builder.addBlockArgument(join,i8);
    auto v=builder.emitValue("literal",{},i8);
    builder.terminateWithSuccessors("jump",{},{{join,{ir::Operand::fromValue(v)}}});
    builder.setCurrentBlock(join);
    builder.emit("consume",{ir::Operand::fromValue(param)});
    builder.terminate("return");
    const auto module=builder.finish();
    ir::validate(module,{true,true,&space});
    CHECK(module.blocks[1].arguments[0].type==i8);
    ir::TypeSpace foreign{"other"};
    CHECK(rejects([&]{ ir::validate(module,{true,true,&foreign}); }));
}

TEST(TypeIdentity, MissingTypeAndUnresolvedPolicies) {
    ir::HIRBuilder builder;
    auto v=builder.emitValue("unknown",{},ir::ValueType::Unknown);
    builder.terminate("return",{ir::Operand::fromValue(v)});
    auto module=builder.finish();
    ir::validate(module,{true});
    CHECK(rejects([&]{ ir::validate(module,{true,true}); }));
    module.blocks[0].instructions[0].type=ir::TypeRef{};
    CHECK(rejects([&]{ ir::validate(module); }));
}

TEST(TypeIdentity, LoweringPreservesCustomTypesAndBlockArguments) {
    ir::TypeSpace space{"example.types"}; auto word=space.intern("Word16");
    ir::HIRBuilder builder;
    auto join=builder.createBlock("join");
    auto param=builder.addBlockArgument(join,word);
    auto value=builder.emitValue("source",{},word);
    builder.terminateWithSuccessors("jump",{},{{join,{ir::Operand::fromValue(value)}}});
    builder.setCurrentBlock(join);
    builder.emit("consume",{ir::Operand::fromValue(param)});
    builder.terminate("return");
    auto module=builder.finish();
    ir::LoweringRegistry registry;
    registry.mir("source",[](const ir::MIRLoweringInput &in,ir::MIRBuilder &out,
                              const ir::MIRLoweringContext &,const ir::LoweringRegistry &)->ir::MIRLoweringResult{
        return out.emitValue("mir.source",in.operands,in.source.type);
    });
    registry.mir("consume",[](const ir::MIRLoweringInput &in,ir::MIRBuilder &out,
                               const ir::MIRLoweringContext &,const ir::LoweringRegistry &)->ir::MIRLoweringResult{
        out.emit("mir.consume",in.operands);return {};
    });
    auto mir=ir::HIRLoweringPass{registry}.lower(module,{{true,true,&space}});
    CHECK(mir.blocks[0].instructions[0].type==word);
    CHECK(mir.blocks[1].arguments[0].type==word);
    CHECK(mir.blocks[0].terminator->successors[0].arguments[0].asValue().value==1);
    ir::validate(mir,{true,true,&space});
}

TEST(TypeIdentity, ConversionRequiresExplicitLoweringResult) {
    ir::TypeSpace space{"convert.types"};
    auto i8=space.intern("i8"), u64=space.intern("u64");
    ir::HIRBuilder builder;
    builder.emitValue("convert",{},i8); builder.terminate("return");
    auto hir=builder.finish();
    ir::LoweringRegistry bad;
    bad.mir("convert",[&](const ir::MIRLoweringInput &,ir::MIRBuilder &out,
                         const ir::MIRLoweringContext &,const ir::LoweringRegistry &)->ir::MIRLoweringResult{
        return out.emitValue("cast",{},u64);
    });
    CHECK(rejects([&]{ (void)ir::HIRLoweringPass{bad}.lower(hir); }));
    ir::LoweringRegistry declared;
    declared.mir("convert",[&](const ir::MIRLoweringInput &,ir::MIRBuilder &out,
                         const ir::MIRLoweringContext &,const ir::LoweringRegistry &)->ir::MIRLoweringResult{
        return ir::MIRLoweringResult::converted(out.emitValue("cast",{},u64));
    });
    CHECK(rejects([&]{ (void)ir::HIRLoweringPass{declared}.lower(hir); }));
    ir::HIRLoweringOptions policy;
    policy.allowsConversion=[&](const ir::TypeRef &source,const ir::TypeRef &target){ return source==i8 && target==u64; };
    CHECK(ir::HIRLoweringPass{declared}.lower(hir,policy).blocks[0].instructions[0].type==u64);
    policy.allowsConversion=[](const ir::TypeRef &,const ir::TypeRef &){return false;};
    CHECK(rejects([&]{ (void)ir::HIRLoweringPass{declared}.lower(hir,policy); }));
}

TEST(TypeIdentity, GenericIRReusesSameScopedReference) {
    ir::TypeSpace s{"generic.example"}; auto token=s.intern("Token");
    ir::generic::TypeRef generic=token;
    CHECK(generic==token);
    ir::generic::OperationRegistry registry;
    registry.add({"typed",0,0,1,1,{},{}});
    ir::generic::Operation op;op.name="typed";op.results={{3}};op.resultTypes={token};
    registry.validate(op);
    op.resultTypes={ir::TypeRef{}};
    CHECK(rejects([&]{registry.validate(op);}));
    op.resultTypes={ir::TypeRef{ir::ValueType::Void}};
    CHECK(rejects([&]{registry.validate(op);}));
}

TEST(TypeIdentity, RejectsForgedBuiltinReferences) {
    CHECK((!ir::TypeRef{"not-a-builtin", "novac.builtin"}.valid()));
    CHECK(rejects([] { (void)ir::TypeRef{"not-a-builtin", "novac.builtin"}.serialize(); }));
    CHECK(ir::TypeRef::dynamic().valid());
    CHECK(!ir::TypeRef::dynamic().unresolved());
}

TEST(TypeIdentity, ExplicitTypeImportsCheckBothRegistries) {
    ir::TypeSpace alpha{"org.lang.alpha"}, beta{"org.lang.beta"};
    const auto x = alpha.intern("x"), y = beta.intern("y");
    ir::TypeImportMap mapping;
    mapping.add(alpha,x,beta,y);
    CHECK(mapping.resolve(x)==y);
    ir::TypeImportMap invalid;
    CHECK(rejects([&]{invalid.add(beta,x,alpha,y);}));
}

TEST(TypeIdentity, GenericIRRequiresOneValidTypePerProducedValue) {
    ir::generic::OperationRegistry registry;
    registry.add({"op",0,0,1,1,{},{}});
    ir::generic::Operation operation;
    operation.name="op";
    operation.results={{3}};
    CHECK(rejects([&]{ registry.validate(operation); }));
    operation.resultTypes={ir::TypeRef{"value", "some.scope"}};
    registry.validate(operation);
}
