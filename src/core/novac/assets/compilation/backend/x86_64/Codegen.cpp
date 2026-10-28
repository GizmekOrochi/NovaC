#include "novac/assets/compilation/backend/x86_64/Codegen.hpp"
#include "novac/assets/compilation/backend/x86_64/StandardRecipes.hpp"
#include "novac/assets/compilation/machine/NativeFramePlan.hpp"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace novac::backend::x86_64 {
namespace {
using compilation::machine::MachineLayout;
using compilation::machine::ScalarKind;
using compilation::machine::MachineOpcode;

class Encoder {
public:
    std::vector<std::uint8_t> code;
    void byte(std::uint8_t b) { code.push_back(b); }
    void bytes(std::initializer_list<std::uint8_t> b) { code.insert(code.end(), b.begin(), b.end()); }
    void u32(std::uint32_t v) { for (int j=0;j<4;++j) byte(static_cast<std::uint8_t>(v >> (8*j))); }
    void u64(std::uint64_t v) { for (int j=0;j<8;++j) byte(static_cast<std::uint8_t>(v >> (8*j))); }
    void patch32(std::size_t at, std::size_t target) {
        const auto distance=static_cast<std::int64_t>(target)-static_cast<std::int64_t>(at+4);
        if (distance<std::numeric_limits<std::int32_t>::min() || distance>std::numeric_limits<std::int32_t>::max())
            throw std::runtime_error("x86-64: rel32 displacement overflow");
        const auto raw=static_cast<std::uint32_t>(static_cast<std::int32_t>(distance));
        for(int j=0;j<4;++j) code.at(at+j)=static_cast<std::uint8_t>(raw>>(8*j));
    }
    std::size_t rel32(std::initializer_list<std::uint8_t> op) {
        bytes(op); const auto pos=code.size(); u32(0); return pos;
    }
    // All stack operands are at [rbp - positive disp32]. We deliberately
    // support only the registers owned by this minimal stack-based selector.
    void disp(std::uint32_t offset) { u32(0-offset); }
    void immediate(std::uint64_t value, std::uint8_t bits) {
        if (bits==64) { bytes({0x48,0xB8}); u64(value); }
        else { byte(0xB8); u32(static_cast<std::uint32_t>(value)); }
    }
    void signExtendEaxToRax() { bytes({0x48,0x98}); } // cdqe
    void load(std::uint32_t off, MachineLayout layout, bool toRdx=false) {
        const std::uint8_t modrm = toRdx ? 0x95 : 0x85;
        if (layout.bits==64) bytes({0x48,0x8B,modrm});
        else if (layout.bits==32) bytes({0x8B,modrm});
        else if (layout.bits==16) bytes({0x0F,layout.kind==ScalarKind::SignedInteger ? std::uint8_t{0xBF} : std::uint8_t{0xB7},modrm});
        else bytes({0x0F,layout.kind==ScalarKind::SignedInteger ? std::uint8_t{0xBE} : std::uint8_t{0xB6},modrm});
        disp(off);
    }
    void store(std::uint32_t off, MachineLayout layout) {
        if (layout.bits==64) bytes({0x48,0x89,0x85});
        else if (layout.bits==32) bytes({0x89,0x85});
        else if (layout.bits==16) bytes({0x66,0x89,0x85});
        else bytes({0x88,0x85});
        disp(off);
    }
    void add(bool wide) { if (wide) byte(0x48); bytes({0x01,0xD0}); } // add [r/e]ax, [r/e]dx
    void sub(bool wide) { if (wide) byte(0x48); bytes({0x29,0xD0}); }
    void mul(bool wide) { if (wide) byte(0x48); bytes({0x0F,0xAF,0xC2}); }
    void lessThan(bool wide, bool isSigned) {
        if (wide) byte(0x48);
        bytes({0x39,0xD0,0x0F,static_cast<std::uint8_t>(isSigned?0x9C:0x92),0xC0,0x0F,0xB6,0xC0});
    }
    void addImmediate(std::uint64_t value, bool wide) {
        if (wide) {
            const auto signedValue=static_cast<std::int64_t>(value);
            if (signedValue < std::numeric_limits<std::int32_t>::min() ||
                signedValue > std::numeric_limits<std::int32_t>::max())
                throw std::runtime_error("x86-64: 64-bit add immediate must fit sign-extended imm32");
            byte(0x48);
        }
        byte(0x05); u32(static_cast<std::uint32_t>(value));
    }
};
using Slot = compilation::machine::ValueSlot;
struct Fixup { std::size_t at; std::size_t block; };

class Selector {
public:
    Selector(const ir::MIRModule &mir, const ValueTypes &types,
             const compilation::machine::MachineRecipeRegistry &recipes,
             const compilation::machine::MachineTarget &target)
        : mir_{mir}, types_{types}, recipes_{recipes}, target_{target} {
        if (!types_.i32.valid() || !types_.boolean.valid() || types_.i32 == types_.boolean ||
            types_.i32.isVoid() || types_.boolean.isVoid() ||
            types_.i32.unresolved() || types_.boolean.unresolved())
            fail("invalid or ambiguous target type mapping");
        frame_ = std::make_unique<compilation::machine::NativeFramePlan>(mir_, makeLayouts(types_));
        labels_.resize(mir_.blocks.size());
        // Per-compilation immutable selection plan. Callbacks and matches are
        // evaluated once, so stateful predicates cannot change the selected
        // recipe between preflight and native code emission.
        selected_.reserve(mir_.blocks.size());
        for (std::size_t b = 0; b < mir_.blocks.size(); ++b) {
            std::vector<const compilation::machine::MachineRecipe *> blockRecipes;
            blockRecipes.reserve(mir_.blocks[b].instructions.size());
            for (std::size_t i = 0; i < mir_.blocks[b].instructions.size(); ++i) {
                const auto &inst = mir_.blocks[b].instructions[i];
                try {
                    blockRecipes.push_back(&recipes_.select(inst, target_, [this](ir::ValueId v)->std::optional<ir::TypeRef> {
                        const auto it = frame_->slots().find(v.value);
                        return it == frame_->slots().end() ? std::nullopt : std::optional<ir::TypeRef>{it->second.type};
                    }));
                } catch (const std::exception &e) {
                    fail("selection, block " + std::to_string(b) + " instruction " + std::to_string(i) +
                         " ('" + inst.op + "'): " + e.what());
                }
            }
            selected_.push_back(std::move(blockRecipes));
        }
    }
    MachineImage generate() {
        const auto call=encoder_.rel32({0xE8});
        encoder_.bytes({0x89,0xC7}); // mov edi,eax
        encoder_.immediate(60,32);    // mov eax, SYS_exit
        encoder_.bytes({0x0F,0x05}); // syscall
        const auto mainOffset=encoder_.code.size();
        encoder_.patch32(call,mainOffset);
        encoder_.bytes({0x55,0x48,0x89,0xE5});
        if (frame_->frameBytes()) { encoder_.bytes({0x48,0x81,0xEC}); encoder_.u32(frame_->frameBytes()); }
        for(std::size_t b=0;b<mir_.blocks.size();++b) {
            labels_[b]=encoder_.code.size();
            const auto &block=mir_.blocks[b];
            for(std::size_t i=0;i<block.instructions.size();++i) {
                const auto &inst=block.instructions[i];
                try { emitInstruction(inst, *selected_[b][i]); }
                catch (const std::exception &e) {
                    fail("block "+std::to_string(b)+" ('"+block.name+"'), instruction "+std::to_string(i)+
                         " ('"+inst.op+"'): "+e.what());
                }
            }
            try { emitTerminator(*block.terminator); }
            catch (const std::exception &e) {
                fail("block "+std::to_string(b)+" ('"+block.name+"'), terminator ('"+
                     block.terminator->op+"'): "+e.what());
            }
        }
        for(const auto &fix : fixups_) encoder_.patch32(fix.at,labels_.at(fix.block));
        return {std::move(encoder_.code),mainOffset};
    }
private:
    const ir::MIRModule &mir_;
    const ValueTypes &types_;
    const compilation::machine::MachineRecipeRegistry &recipes_;
    const compilation::machine::MachineTarget &target_;
    std::unique_ptr<compilation::machine::NativeFramePlan> frame_{};
    Encoder encoder_{};
    std::vector<std::size_t> labels_{};
    std::vector<Fixup> fixups_{};
    std::vector<std::vector<const compilation::machine::MachineRecipe *>> selected_{};
    static compilation::machine::MachineLayoutRegistry makeLayouts(const ValueTypes& types) {
        compilation::machine::MachineLayoutRegistry layouts;
        layouts.add(types.i32, {32,ScalarKind::SignedInteger,4});
        layouts.add(types.boolean, {8,ScalarKind::Boolean,1});
        for (const auto &[type, layout] : types.layouts.entries()) layouts.add(type,layout);
        return layouts;
    }
    [[noreturn]] static void fail(const std::string &message) {
        throw std::runtime_error("X86_64Backend: "+message);
    }
    const Slot &slot(const ir::Operand &operand, const ir::TypeRef &type) const {
        if (!operand.isValue()) fail("expected SSA value operand");
        const auto it=frame_->slots().find(operand.asValue().value);
        if (it==frame_->slots().end()) fail("undefined SSA value");
        if (it->second.type!=type) fail("operand type mismatch");
        return it->second;
    }
    static void verifyImmediate(const compilation::machine::MachineInstruction &m,
                                const MachineLayout &layout, const std::string &recipe) {
        if (m.opcode != MachineOpcode::LoadImmediate) return;
        if (layout.kind == ScalarKind::Boolean) {
            if (m.immediate > 1) fail("recipe '" + recipe + "': noncanonical boolean immediate");
            return;
        }
        // A constant must fit its destination *exactly*. Truncation requires
        // an explicit conversion operation, never an implicit 8/16-bit store.
        if (layout.bits == 64) {
            if (layout.kind == ScalarKind::UnsignedInteger && m.signedImmediate &&
                std::bit_cast<std::int64_t>(m.immediate) < 0)
                fail("recipe '" + recipe + "': negative constant out of range for unsigned 64-bit type");
            if (layout.kind == ScalarKind::SignedInteger && !m.signedImmediate &&
                m.immediate > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
                fail("recipe '" + recipe + "': unsigned constant out of range for signed 64-bit type");
            return;
        }
        if (layout.kind == ScalarKind::UnsignedInteger) {
            if ((m.signedImmediate && std::bit_cast<std::int64_t>(m.immediate) < 0) ||
                m.immediate > ((std::uint64_t{1} << layout.bits) - 1))
                fail("recipe '" + recipe + "': integer constant out of range for unsigned " +
                     std::to_string(layout.bits) + "-bit type");
        } else if (layout.kind == ScalarKind::SignedInteger) {
            const auto max = (std::uint64_t{1} << (layout.bits - 1)) - 1;
            const auto min = -static_cast<std::int64_t>(std::uint64_t{1} << (layout.bits - 1));
            if (m.signedImmediate) {
                const auto number = std::bit_cast<std::int64_t>(m.immediate);
                if (number < min || number > static_cast<std::int64_t>(max))
                    fail("recipe '" + recipe + "': integer constant out of range for signed " +
                         std::to_string(layout.bits) + "-bit type");
            } else if (m.immediate > max) {
                fail("recipe '" + recipe + "': unsigned constant out of range for signed " +
                     std::to_string(layout.bits) + "-bit type");
            }
        }
    }
    void emitInstruction(const ir::Instruction &inst, const compilation::machine::MachineRecipe &recipe) {
        if (!inst.result) {
            compilation::machine::MachineBuilder builder{inst};
            try { recipe.emit(inst,builder); }
            catch (const std::exception &e) {
                fail("recipe '"+recipe.id+"' ("+recipe.owner+") for void operation: "+e.what());
            }
            const auto machine=builder.finish();
            for (const auto &item : machine.instructions) {
                switch(item.opcode) {
                    case MachineOpcode::Nop: encoder_.byte(0x90); break;
                    case MachineOpcode::Trap: encoder_.bytes({0x0F,0x0B}); break;
                    default: fail("effect-only recipe must emit effect-only instructions");
                }
            }
            return;
        }
        const auto &dest=frame_->require(ir::ValueId{inst.result->value});
        compilation::machine::MachineIR code;
        try {
            compilation::machine::MachineBuilder builder{inst};
            recipe.emit(inst,builder);
            code=builder.finish();
        } catch(const std::exception &e) {
            fail("recipe '"+recipe.id+"' ("+recipe.owner+") for '"+inst.op+"': "+e.what());
        }
        ir::TypeRef accumulator{ir::ValueType::Unknown};
        for(const auto &m : code.instructions) {
            switch(m.opcode) {
                case MachineOpcode::LoadImmediate:
                    verifyImmediate(m, dest.layout, recipe.id);
                    accumulator=dest.type; break;
                case MachineOpcode::LoadSSA:
                case MachineOpcode::ConvertSSA: {
                    if (!m.source || !frame_->slots().contains(m.source->value)) fail("recipe references undefined SSA value");
                    const auto &source=frame_->require(ir::ValueId{m.source->value});
                    if (m.opcode==MachineOpcode::ConvertSSA) {
                        if (source.layout.kind==ScalarKind::Boolean || dest.layout.kind==ScalarKind::Boolean)
                            fail("integer conversion cannot silently convert boolean");
                        accumulator=dest.type;
                    } else accumulator=source.type;
                    break;
                }
                case MachineOpcode::AddSSA: case MachineOpcode::SubSSA: case MachineOpcode::MulSSA:
                case MachineOpcode::LessThanSSA: case MachineOpcode::UnsignedLessThanSSA: {
                    if (!m.source || !frame_->slots().contains(m.source->value) ||
                        frame_->require(ir::ValueId{m.source->value}).type!=accumulator ||
                        accumulator==types_.boolean || !frame_->layouts().has(accumulator) ||
                        frame_->layouts().require(accumulator).kind==ScalarKind::Boolean)
                        fail("recipe '"+recipe.id+"': incompatible arithmetic operand types");
                    const auto kind=frame_->layouts().require(accumulator).kind;
                    if ((m.opcode==MachineOpcode::LessThanSSA && kind!=ScalarKind::SignedInteger) ||
                        (m.opcode==MachineOpcode::UnsignedLessThanSSA && kind!=ScalarKind::UnsignedInteger))
                        fail("recipe '"+recipe.id+"': comparison signedness mismatch");
                    if (m.opcode==MachineOpcode::LessThanSSA || m.opcode==MachineOpcode::UnsignedLessThanSSA)
                        accumulator=types_.boolean;
                    break;
                }
                case MachineOpcode::Nop: case MachineOpcode::Trap:
                    fail("value-producing recipe contains effect-only machine operation");
                case MachineOpcode::AddImmediate:
                    if (!frame_->layouts().has(accumulator) || frame_->layouts().require(accumulator).kind==ScalarKind::Boolean)
                        fail("recipe '"+recipe.id+"': add immediate requires integer accumulator");
                    break;
            }
        }
        if (accumulator!=dest.type)
            fail("recipe '"+recipe.id+"': result type does not match MIR result");
        // The full recipe has passed type-state checks before emitting bytes.
        ir::TypeRef current{ir::ValueType::Unknown};
        for(const auto &m : code.instructions) {
            switch(m.opcode) {
                case MachineOpcode::LoadImmediate:
                    encoder_.immediate(m.immediate,dest.layout.bits);
                    current=dest.type;
                    break;
                case MachineOpcode::LoadSSA: case MachineOpcode::ConvertSSA:
                    encoder_.load(frame_->require(ir::ValueId{m.source->value}).offset, frame_->require(ir::ValueId{m.source->value}).layout);
                    if (m.opcode==MachineOpcode::ConvertSSA && dest.layout.bits==64 &&
                        frame_->require(ir::ValueId{m.source->value}).layout.bits<64 &&
                        frame_->require(ir::ValueId{m.source->value}).layout.kind==ScalarKind::SignedInteger)
                        encoder_.signExtendEaxToRax();
                    current=m.opcode==MachineOpcode::ConvertSSA ? dest.type : frame_->require(ir::ValueId{m.source->value}).type;
                    break;
                case MachineOpcode::AddSSA: case MachineOpcode::SubSSA: case MachineOpcode::MulSSA:
                case MachineOpcode::LessThanSSA: case MachineOpcode::UnsignedLessThanSSA: {
                    const auto &source=frame_->require(ir::ValueId{m.source->value});
                    encoder_.load(source.offset,source.layout,true);
                    const bool wide=source.layout.bits==64;
                    switch(m.opcode) {
                        case MachineOpcode::AddSSA: encoder_.add(wide); break;
                        case MachineOpcode::SubSSA: encoder_.sub(wide); break;
                        case MachineOpcode::MulSSA: encoder_.mul(wide); break;
                        case MachineOpcode::LessThanSSA: encoder_.lessThan(wide,true);break;
                        default: encoder_.lessThan(wide,false);break;
                    }
                    if (m.opcode==MachineOpcode::LessThanSSA || m.opcode==MachineOpcode::UnsignedLessThanSSA)
                        current=types_.boolean;
                    break;
                }
                case MachineOpcode::Nop: case MachineOpcode::Trap:
                    fail("effect-only machine operation within value recipe");
                case MachineOpcode::AddImmediate:
                    encoder_.addImmediate(m.immediate,frame_->layouts().require(current).bits==64); break;
            }
        }
        encoder_.store(dest.offset,dest.layout);
    }
    void jump(std::size_t target) { fixups_.push_back({encoder_.rel32({0xE9}),target}); }
    void copyEdge(const ir::Successor &edge) {
        const auto &target=mir_.blocks.at(edge.target.value);
        if (edge.arguments.size()!=target.arguments.size()) fail("edge argument arity mismatch");
        // Parallel copies: 8-byte scratch per argument to avoid both source
        // clobbering and width mismatch between neighboring stack slots.
        for(std::size_t i=0;i<edge.arguments.size();++i) {
            const auto &param=target.arguments[i];
            const auto &src=slot(edge.arguments[i],param.type);
            encoder_.load(src.offset,src.layout);
            encoder_.store(static_cast<std::uint32_t>(frame_->scratchStart()+(i+1)*8),{64,ScalarKind::UnsignedInteger,8});
        }
        for(std::size_t i=0;i<edge.arguments.size();++i) {
            encoder_.load(static_cast<std::uint32_t>(frame_->scratchStart()+(i+1)*8),{64,ScalarKind::UnsignedInteger,8});
            const auto &dst=frame_->require(ir::ValueId{target.arguments[i].value.value});
            encoder_.store(dst.offset,dst.layout);
        }
        jump(edge.target.value);
    }
    void emitTerminator(const ir::Terminator &term) {
        if (term.op=="return") {
            if (!term.successors.empty() || term.operands.size()!=1) fail("return requires one SSA value");
            if (!term.operands[0].isValue()) fail("return requires SSA value");
            const auto it=frame_->slots().find(term.operands[0].asValue().value);
            if (it==frame_->slots().end() || it->second.layout.kind==ScalarKind::Boolean)
                fail("return operand type mismatch: requires integer SSA value");
            encoder_.load(it->second.offset,it->second.layout);
            encoder_.bytes({0xC9,0xC3});
        } else if (term.op=="jump") {
            if (!term.operands.empty() || term.successors.size()!=1) fail("jump requires one edge");
            copyEdge(term.successors[0]);
        } else if (term.op=="branch") {
            if (term.operands.size()!=1 || term.successors.size()!=2)
                fail("branch requires bool condition and two edges");
            const auto &condition=slot(term.operands[0],types_.boolean);
            encoder_.load(condition.offset,condition.layout);
            encoder_.bytes({0x85,0xC0});
            const auto skip=encoder_.rel32({0x0F,0x84});
            copyEdge(term.successors[0]);
            encoder_.patch32(skip,encoder_.code.size());
            copyEdge(term.successors[1]);
        } else fail("unsupported terminator '"+term.op+"'");
    }
};
} // namespace

MachineImage selectAndEncode(const ir::MIRModule &module, const ValueTypes &types,
                             const compilation::machine::MachineRecipeRegistry &recipes,
                             const compilation::machine::MachineTarget &target) {
    return Selector{module,types,recipes,target}.generate();
}
MachineImage selectAndEncode(const ir::MIRModule &module, const ValueTypes &types) {
    return selectAndEncode(module,types,standardRecipes(types),{});
}
} // namespace novac::backend::x86_64
