#include "../../tester.hpp"
#include "novac/engine/transformation/IR.hpp"
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
using namespace novac::ir;
using V = ValueType;
using O = Operand;
using S = Successor;
using R = MIRLoweringResult;
using I = MIRLoweringInput;
using C = MIRLoweringContext;
using L = LoweringRegistry;
template<class F> std::string failure(F &&fn) {
    try { fn(); } catch (const std::runtime_error &e) { return e.what(); }
    return {};
}
void basicLowerers(L &registry) {
    registry.mir("value", [](const I &in, MIRBuilder &out, const C &, const L &)->R {
        return out.emitValue("mir.value", in.operands, in.source.type);
    });
    registry.mir("consume", [](const I &in, MIRBuilder &out, const C &, const L &)->R {
        out.emit("mir.consume", in.operands); return {};
    });
}
}

TEST(SSA, DiamondWithBlockArgumentLowersToMIR) {
    L lowerers; basicLowerers(lowerers);
    HIRBuilder h;
    auto left=h.createBlock("left"), right=h.createBlock("right"), join=h.createBlock("join");
    auto arg=h.addBlockArgument(join,V::Int);
    h.terminate("branch", {}, {left,right});
    h.setCurrentBlock(left);
    auto x=h.emitValue("value",{},V::Int);
    h.terminateWithSuccessors("jump",{}, {S{join,{O::fromValue(x)}}});
    h.setCurrentBlock(right);
    auto y=h.emitValue("value",{},V::Int);
    h.terminateWithSuccessors("jump",{}, {S{join,{O::fromValue(y)}}});
    h.setCurrentBlock(join);
    h.emit("consume",{O::fromValue(arg)}); h.terminate("return");
    auto source=h.finish(); validate(source, {true});
    auto m=HIRLoweringPass{lowerers}.lower(source, {{true}});
    CHECK(m.blocks[join.value].arguments.size()==1);
    auto target=m.blocks[join.value].arguments[0].value;
    CHECK(m.blocks[join.value].instructions[0].operands[0].asValue().value==target.value);
    CHECK(m.blocks[left.value].terminator->successors[0].arguments[0].asValue().value ==
          m.blocks[left.value].instructions[0].result->value);
    CHECK(m.blocks[right.value].terminator->successors[0].arguments[0].asValue().value ==
          m.blocks[right.value].instructions[0].result->value);
}


TEST(SSA, BackedgeAndLoopHeaderArgumentsLower) {
    L lowerers; basicLowerers(lowerers);
    HIRBuilder h;
    auto loop=h.createBlock("loop"), body=h.createBlock("body"), exit=h.createBlock("exit");
    auto i=h.addBlockArgument(loop,V::Int);
    auto exitValue=h.addBlockArgument(exit,V::Int);
    auto initial=h.emitValue("value",{},V::Int);
    h.terminateWithSuccessors("jump",{}, {S{loop,{O::fromValue(initial)}}});
    h.setCurrentBlock(loop);
    h.terminateWithSuccessors("branch", {}, {S{body},S{exit,{O::fromValue(i)}}});
    h.setCurrentBlock(body);
    auto next=h.emitValue("value",{O::fromValue(i)},V::Int);
    h.terminateWithSuccessors("jump",{}, {S{loop,{O::fromValue(next)}}});
    h.setCurrentBlock(exit);
    h.emit("consume",{O::fromValue(exitValue)});h.terminate("return");
    auto m=HIRLoweringPass(lowerers).lower(h.finish(), {{true}});
    CHECK(m.blocks[loop.value].arguments.size()==1);
    CHECK(m.blocks[exit.value].arguments.size()==1);
    CHECK(m.blocks[body.value].terminator->successors[0].arguments[0].asValue().value ==
          m.blocks[body.value].instructions[0].result->value);
    CHECK(m.blocks[loop.value].terminator->successors[1].arguments[0].asValue().value ==
          m.blocks[loop.value].arguments[0].value.value);
}

TEST(SSA, TwoEdgesToSameBlockCarryDifferentArguments) {
    HIRBuilder h;
    auto join=h.createBlock("join");h.addBlockArgument(join,V::Int);
    auto x=h.emitValue("noop",{},V::Int),y=h.emitValue("noop",{},V::Int);
    h.terminateWithSuccessors("branch",{}, {S{join,{O::fromValue(x)}},S{join,{O::fromValue(y)}}});
    h.setCurrentBlock(join);h.terminate("return");
    auto module=h.finish();
    const auto &edges=module.blocks[0].terminator->successors;
    CHECK(edges.size()==2);
    CHECK(edges[0].target.value==edges[1].target.value);
    CHECK(edges[0].arguments[0].asValue().value!=edges[1].arguments[0].asValue().value);
}

TEST(SSA, DuplicateValueIdsRejectAcrossParametersAndInstructions) {
    HIRModule m;
    m.blocks.push_back({BlockId{0},"entry"});m.blocks.push_back({BlockId{1},"target"});
    m.blocks[0].terminator=Terminator{"jump",{}, {S{BlockId{1},{O::fromValue(ValueId{9})}}}};
    m.blocks[0].instructions.push_back({"value",ValueId{9},{},V::Int});
    m.blocks[1].arguments.push_back({ValueId{9},V::Int});
    CHECK(failure([&]{validate(m);}).find("duplicate value")!=std::string::npos);
}

TEST(SSA, EdgeArityMismatchIsRejected) {
    HIRBuilder h;
    auto target=h.createBlock("join");h.addBlockArgument(target,V::Int);
    h.terminate("jump",{}, {target});
    CHECK(failure([&]{h.finish();}).find("wrong argument count")!=std::string::npos);
}

TEST(SSA, EdgeTypeMismatchIsRejected) {
    HIRBuilder h;
    auto dest=h.createBlock("join");h.addBlockArgument(dest,V::Bool);
    auto v=h.emitValue("value",{},V::Int);
    h.terminateWithSuccessors("jump",{}, {S{dest,{O::fromValue(v)}}});
    CHECK(failure([&]{h.finish();}).find("argument type")!=std::string::npos);
}

TEST(SSA, EdgePayloadMustReferenceAValueNotLiteralOrSymbol) {
    for (auto op : {O::fromLiteral(4), O::fromSymbol("x")}) {
        HIRBuilder h;auto dest=h.createBlock("join");h.addBlockArgument(dest,V::Int);
        h.terminateWithSuccessors("jump",{}, {S{dest,{op}}});
        CHECK(failure([&]{h.finish();}).find("SSA values")!=std::string::npos);
    }
}

TEST(SSA, EdgeSourceAvailabilityAndDominanceChecked) {
    HIRBuilder h;
    auto left=h.createBlock("left"), right=h.createBlock("right"), join=h.createBlock("join");
    h.addBlockArgument(join,V::Int);
    h.terminate("branch",{}, {left,right});
    h.setCurrentBlock(left);auto v=h.emitValue("value",{},V::Int);h.terminate("return");
    h.setCurrentBlock(right);h.terminateWithSuccessors("jump",{}, {S{join,{O::fromValue(v)}}});
    h.setCurrentBlock(join);h.terminate("return");
    CHECK(failure([&]{h.finish();}).find("does not dominate")!=std::string::npos);
}

TEST(SSA, LoopHeaderParameterIsAvailableFromFirstInstruction) {
    HIRBuilder h;
    auto loop=h.createBlock("loop");auto arg=h.addBlockArgument(loop,V::Int);
    auto init=h.emitValue("value",{},V::Int);
    h.terminateWithSuccessors("jump",{}, {S{loop,{O::fromValue(init)}}});
    h.setCurrentBlock(loop);h.emit("inspect",{O::fromValue(arg)});
    h.terminateWithSuccessors("jump",{}, {S{loop,{O::fromValue(arg)}}});
    CHECK(h.finish().blocks[loop.value].instructions[0].operands[0].asValue().value==arg.value);
}

TEST(SSA, EntryParameterForbiddenWithoutExternalSignature) {
    HIRModule m; m.blocks.push_back({BlockId{0},"entry"});
    m.blocks[0].arguments.push_back({ValueId{0},V::Int});
    CHECK(failure([&]{validate(m);}).find("external signature")!=std::string::npos);
    HIRBuilder builder;
    CHECK(failure([&]{builder.addBlockArgument(BlockId{0},V::Int);}).find("entry signature")!=std::string::npos);
}

TEST(SSA, ParameterTypeVoidRejectedUnknownAllowedOnlyWhenMatched) {
    HIRBuilder h;auto dest=h.createBlock("dest");
    CHECK(failure([&]{h.addBlockArgument(dest,V::Void);}).find("void type")!=std::string::npos);
    auto arg=h.addBlockArgument(dest,V::Unknown);(void)arg;
    auto v=h.emitValue("value",{},V::Unknown);
    h.terminateWithSuccessors("jump",{}, {S{dest,{O::fromValue(v)}}});
    h.setCurrentBlock(dest);h.terminate("return");
    CHECK(h.finish().blocks[dest.value].arguments[0].type==V::Unknown);
}

TEST(SSA, BuilderRejectsParameterAddedAfterIncomingEdgeAndAfterBody) {
    HIRBuilder h;
    auto dest=h.createBlock("dest");h.terminate("jump",{}, {dest});
    CHECK(!failure([&]{h.addBlockArgument(dest,V::Int);}).empty());
    h.setCurrentBlock(dest);h.emit("side.effect");
    CHECK(!failure([&]{h.addBlockArgument(dest,V::Int);}).empty());
}

TEST(SSA, InvalidSuccessorDoesNotMutateBuilder) {
    HIRBuilder h;
    CHECK(!failure([&]{h.terminateWithSuccessors("jump", {}, {S{BlockId{77}}});}).empty());
    h.terminate("return"); CHECK(h.finish().blocks[0].terminator->op=="return");
}

TEST(SSA, UnreachableBlockParametersAcceptedAndLocalUsesSafe) {
    HIRBuilder h;
    auto dead=h.createBlock("unreachable");auto arg=h.addBlockArgument(dead,V::Int);
    h.terminate("return");
    h.setCurrentBlock(dead);h.emit("side",{O::fromValue(arg)});h.terminate("return");
    CHECK(h.finish().blocks[dead.value].arguments.size()==1);
}

TEST(SSA, MIRLowererCanAliasAlreadyAllocatedBlockParameter) {
    L reg; reg.mir("alias",[](const I& input,MIRBuilder&,const C&,const L&)->R {
        return input.operands[0].asValue();
    });
    HIRBuilder h;
    auto target=h.createBlock("target");auto arg=h.addBlockArgument(target,V::Int);
    auto source=h.emitValue("literal",{},V::Int);
    h.terminateWithSuccessors("jump",{}, {S{target,{O::fromValue(source)}}});
    h.setCurrentBlock(target);
    auto copy=h.emitValue("alias", {O::fromValue(arg)},V::Int);
    h.terminateWithSuccessors("jump",{}, {S{target,{O::fromValue(copy)}}});
    reg.mir("literal",[](const I&,MIRBuilder& out,const C&,const L&)->R {
        return out.emitValue("lit",{},V::Int);
    });
    auto m=HIRLoweringPass(reg).lower(h.finish(),{{true}});
    CHECK(m.blocks[target.value].arguments.size()==1);
    CHECK(m.blocks[target.value].terminator->successors[0].arguments[0].asValue().value ==
          m.blocks[target.value].arguments[0].value.value);
}

TEST(SSA, MetadataOnBlockParametersSurvivesLowering) {
    L reg;
    HIRBuilder h;
    auto block=h.createBlock("block");auto arg=h.addBlockArgument(block,V::Int);
    auto init=h.emitValue("constant",{},V::Int);
    h.terminateWithSuccessors("jump",{}, {S{block,{O::fromValue(init)}}});
    h.setCurrentBlock(block);h.terminate("return");
    reg.mir("constant",[](const I&,MIRBuilder& m,const C&,const L&)->R {
        return m.emitValue("mir.constant",{},V::Int);
    });
    auto module=h.finish();module.blocks[block.value].arguments[0].metadata.set<int>("origin",54);
    auto mir=HIRLoweringPass(reg).lower(module);
    CHECK(mir.blocks[block.value].arguments[0].value.value == 0); // allocated before MIR instructions
    (void)arg;
    CHECK(mir.blocks[block.value].arguments[0].metadata.require<int>("origin")==54);
    CHECK(mir.blocks[0].terminator->successors[0].arguments[0].asValue().value==
          mir.blocks[0].instructions[0].result->value);
}

TEST(SSA, SimultaneousSwapSemanticsOnBackedge) {
    HIRBuilder h;
    auto loop=h.createBlock("swap");auto a=h.addBlockArgument(loop,V::Int),b=h.addBlockArgument(loop,V::Int);
    auto v1=h.emitValue("initial",{},V::Int), v2=h.emitValue("initial",{},V::Int);
    h.terminateWithSuccessors("jump",{}, {S{loop,{O::fromValue(v1),O::fromValue(v2)}}});
    h.setCurrentBlock(loop);
    h.terminateWithSuccessors("jump",{}, {S{loop,{O::fromValue(b),O::fromValue(a)}}});
    auto m=h.finish();
    const auto &params=m.blocks[loop.value].arguments;
    const auto &edge=m.blocks[loop.value].terminator->successors.front();
    std::unordered_map<std::uint32_t,int> locals{{a.value,10},{b.value,20}};
    // Reference edge-executor: snapshot all edge values BEFORE any parameter write.
    std::vector<int> incoming;
    for (const auto &op:edge.arguments) incoming.push_back(locals.at(op.asValue().value));
    for (std::size_t i=0;i<incoming.size();++i) locals[params[i].value.value]=incoming[i];
    CHECK(locals[a.value]==20 && locals[b.value]==10);
}

TEST(SSA, BranchTypedBoolAndUnknownAcceptedButIntRejected) {
    for (V type: {V::Bool,V::Unknown}) {
        HIRBuilder h;auto l=h.createBlock("left"),r=h.createBlock("right");
        auto cond=h.emitValue("condition",{},type);
        h.terminate("branch", {O::fromValue(cond)}, {l,r});
        h.setCurrentBlock(l);h.terminate("return");
        h.setCurrentBlock(r);h.terminate("return");
        CHECK(h.finish().blocks.size()==3);
    }
    HIRBuilder h;auto l=h.createBlock("left"),r=h.createBlock("right");
    auto cond=h.emitValue("bad",{},V::Int);
    h.terminate("branch", {O::fromValue(cond)}, {l,r});
    CHECK(failure([&]{h.finish();}).find("branch condition")!=std::string::npos);
}

TEST(SSA, BranchLiteralConditionMustBeBool) {
    HIRBuilder h;auto l=h.createBlock("left"),r=h.createBlock("right");
    h.terminate("branch", {O::fromLiteral(42)}, {l,r});
    CHECK(failure([&]{h.finish();}).find("literal must be bool")!=std::string::npos);
    HIRBuilder correct;auto x=correct.createBlock("left"),y=correct.createBlock("right");
    correct.terminate("branch", {O::fromLiteral(true)}, {x,y});
    correct.setCurrentBlock(x);correct.terminate("return");
    correct.setCurrentBlock(y);correct.terminate("return");
    CHECK(correct.finish().blocks[0].terminator->successors.size()==2);
}

TEST(SSA, PerEdgeMetadataPreservedAndMayBeDisabled) {
    L reg;basicLowerers(reg);
    HIRBuilder h;
    auto next=h.createBlock("next");auto arg=h.addBlockArgument(next,V::Int);
    auto init=h.emitValue("value",{},V::Int);
    S edge{next,{O::fromValue(init)}};edge.metadata.set<int>("edge.origin",123);
    h.terminateWithSuccessors("jump",{}, {edge});
    h.setCurrentBlock(next);h.emit("consume",{O::fromValue(arg)});h.terminate("return");
    auto source=h.finish();
    auto mir=HIRLoweringPass(reg).lower(source,{{true}});
    CHECK(mir.blocks[0].terminator->successors[0].metadata.require<int>("edge.origin")==123);
    auto disabled=HIRLoweringPass(reg).lower(source,{{true},false});
    CHECK(disabled.blocks[0].terminator->successors[0].metadata.empty());
}

TEST(SSA, ModuleValidationCatchesDuplicateParameterIdsWithinBlock) {
    HIRModule m; m.blocks.push_back({BlockId{0},"entry"});m.blocks.push_back({BlockId{1},"other"});
    m.blocks[1].arguments.push_back({ValueId{3},V::Int});
    m.blocks[1].arguments.push_back({ValueId{3},V::Int});
    CHECK(failure([&]{validate(m);}).find("duplicate value")!=std::string::npos);
}

TEST(SSA, LegacyTerminatorRejectsParameterArityMismatch) {
    MIRBuilder m;
    auto next=m.createBlock("next",{V::Int});
    m.terminate("jump",{}, {next});
    CHECK(failure([&]{m.finish();}).find("wrong argument count")!=std::string::npos);
}

TEST(SSA, UnreachableLoopWithBlockParameterLowers) {
    L reg;basicLowerers(reg);
    HIRBuilder h;
    auto dead=h.createBlock("dead");auto i=h.addBlockArgument(dead,V::Int);
    h.terminate("return");
    h.setCurrentBlock(dead);h.emit("consume",{O::fromValue(i)});
    h.terminateWithSuccessors("jump",{}, {S{dead,{O::fromValue(i)}}});
    auto lowered=HIRLoweringPass(reg).lower(h.finish(),{{true}});
    CHECK(lowered.blocks[dead.value].arguments.size()==1);
}

TEST(SSA, WrongEdgePayloadAndMissingTerminatorBothDiagnosed) {
    HIRModule m;
    m.blocks.push_back({BlockId{0},"entry"});
    m.blocks.push_back({BlockId{1},"next"});
    m.blocks[1].arguments.push_back({ValueId{5},V::Bool});
    m.blocks[0].terminator = Terminator{"jump",{}, {S{BlockId{1},{O::fromValue(ValueId{99})}}}};
    CHECK(failure([&]{validate(m);}).find("undefined value")!=std::string::npos);
    m.blocks[0].instructions.push_back({"const",ValueId{99},{},V::Bool});
    CHECK(failure([&]{validate(m,{true});}).find("missing terminator")!=std::string::npos);
}
