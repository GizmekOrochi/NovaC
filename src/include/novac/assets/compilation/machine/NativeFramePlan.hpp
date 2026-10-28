#pragma once
#include "novac/assets/compilation/machine/MachineLayout.hpp"
#include "novac/engine/transformation/IR.hpp"

#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace novac::compilation::machine {

/** Target-independent stack slot accounting for the initial stack-based native
 * pipeline. The target owns instruction encoding, physical stack registers,
 * prologues and epilogues. This planner never emits machine instructions. */
struct FramePolicy {
    std::uint32_t stackAlignment{16};
    std::uint32_t edgeScratchBytes{8};
    std::uint64_t maxFrameBytes{0x7ffffff0ULL};
};
struct ValueSlot {
    std::uint32_t offset{};
    ir::TypeRef type;
    MachineLayout layout;
};
class NativeFramePlan final {
public:
    NativeFramePlan(const ir::MIRModule& mir, MachineLayoutRegistry layouts,
                    FramePolicy policy = {});
    const ValueSlot& require(ir::ValueId value) const;
    const MachineLayoutRegistry& layouts() const noexcept { return layouts_; }
    const std::unordered_map<std::uint32_t,ValueSlot>& slots() const noexcept { return slots_; }
    std::uint32_t scratchStart() const noexcept { return scratchStart_; }
    std::uint32_t frameBytes() const noexcept { return frameBytes_; }
    std::size_t maxEdgeArguments() const noexcept { return maxEdgeArguments_; }
private:
    MachineLayoutRegistry layouts_;
    std::unordered_map<std::uint32_t,ValueSlot> slots_;
    std::size_t maxEdgeArguments_{0};
    std::uint32_t scratchStart_{0};
    std::uint32_t frameBytes_{0};
};
} // namespace novac::compilation::machine
