#include "novac/assets/compilation/machine/NativeFramePlan.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace novac::compilation::machine {
namespace {
std::uint64_t alignUp(std::uint64_t n, std::uint32_t alignment) {
    return (n + alignment - 1) & ~static_cast<std::uint64_t>(alignment - 1);
}
}
NativeFramePlan::NativeFramePlan(const ir::MIRModule& mir, MachineLayoutRegistry layouts, FramePolicy policy)
    : layouts_{std::move(layouts)} {
    const auto isPowerOfTwo = [](std::uint32_t n) { return n && (n & (n-1)) == 0; };
    if (!isPowerOfTwo(policy.stackAlignment) || !policy.edgeScratchBytes ||
        policy.maxFrameBytes > std::numeric_limits<std::uint32_t>::max())
        throw std::runtime_error("NativeFramePlan: invalid target frame policy");
    std::uint64_t used = 0;
    const auto allocate = [&](ir::ValueId value, const ir::TypeRef& type) {
        if (!layouts_.has(type))
            throw std::runtime_error("NativeFramePlan: unsupported SSA type (no machine layout) '" + type.display() + "'");
        const auto layout = layouts_.require(type);
        const auto ending = alignUp(used + layout.bytes(), layout.alignment);
        if (ending > policy.maxFrameBytes)
            throw std::runtime_error("NativeFramePlan: frame exceeds target stack displacement limit");
        used = ending;
        if (!slots_.emplace(value.value, ValueSlot{static_cast<std::uint32_t>(used),type,layout}).second)
            throw std::runtime_error("NativeFramePlan: duplicate SSA value");
    };
    for (const auto &block : mir.blocks) {
        for (const auto &arg : block.arguments) allocate(arg.value,arg.type);
        for (const auto &inst : block.instructions) if (inst.result) allocate(*inst.result,inst.type);
        maxEdgeArguments_ = std::max(maxEdgeArguments_, block.arguments.size());
    }
    if (maxEdgeArguments_ > (policy.maxFrameBytes - used) / policy.edgeScratchBytes)
        throw std::runtime_error("NativeFramePlan: edge scratch exceeds target stack displacement limit");
    const auto scratch = static_cast<std::uint64_t>(maxEdgeArguments_) * policy.edgeScratchBytes;
    if (scratch > policy.maxFrameBytes - used)
        throw std::runtime_error("NativeFramePlan: edge scratch exceeds target stack displacement limit");
    const auto frame = alignUp(used + scratch, policy.stackAlignment);
    if (frame > policy.maxFrameBytes)
        throw std::runtime_error("NativeFramePlan: aligned frame exceeds target stack displacement limit");
    scratchStart_ = static_cast<std::uint32_t>(used);
    frameBytes_ = static_cast<std::uint32_t>(frame);
}
const ValueSlot& NativeFramePlan::require(ir::ValueId value) const {
    const auto it = slots_.find(value.value);
    if (it == slots_.end()) throw std::runtime_error("NativeFramePlan: undefined SSA value");
    return it->second;
}
} // namespace novac::compilation::machine
