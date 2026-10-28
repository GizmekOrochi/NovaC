#include "novac/assets/compilation/backend/CSourceBackend.hpp"
#include "novac/engine/transformation/IR.hpp"

#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace novac::backend {

namespace {
std::string value(const ir::Operand &operand, const std::unordered_set<std::uint32_t> &defined) {
    if (!operand.isValue() || !defined.contains(operand.asValue().value))
        throw std::runtime_error("CSourceBackend: expected a defined SSA value");
    return "v" + std::to_string(operand.asValue().value);
}
}

void CSourceBackend::run(compilation::CompilationSession &session) const {
    const auto &mir = session.artifacts().require<ir::MIRModule>("mir");
    ir::validate(mir, {.requireTerminators = true});
    if (mir.blocks.size() != 1 || !mir.blocks.front().arguments.empty()) {
        throw std::runtime_error("CSourceBackend: prototype supports exactly one block without parameters");
    }
    const auto &block = mir.blocks.front();
    if (!block.terminator || block.terminator->op != "return" ||
        !block.terminator->successors.empty() || block.terminator->operands.size() != 1) {
        throw std::runtime_error("CSourceBackend: expected return of exactly one value");
    }
    std::ostringstream code;
    code << "#include <stdint.h>\nint main(void) {\n";
    std::unordered_set<std::uint32_t> defined;
    for (const auto &inst : block.instructions) {
        if (!inst.result || inst.type != ir::TypeRef{"i32", "novac.prototype"}) {
            throw std::runtime_error("CSourceBackend: only i32 results are supported");
        }
        std::string rhs;
        if (inst.op == "novac.i32.const") {
            if (inst.operands.size() != 1) throw std::runtime_error("CSourceBackend: const expects one operand");
            const auto *literal = std::get_if<ir::Literal>(&inst.operands[0].data());
            if (!literal || !std::holds_alternative<int>(literal->data()))
                throw std::runtime_error("CSourceBackend: const expects a signed integer literal");
            rhs = std::to_string(static_cast<std::uint32_t>(std::get<int>(literal->data()))) + "u";
        } else if (inst.op == "novac.i32.add") {
            if (inst.operands.size() != 2) throw std::runtime_error("CSourceBackend: add expects two operands");
            rhs = value(inst.operands[0], defined) + " + " + value(inst.operands[1], defined);
        } else {
            throw std::runtime_error("CSourceBackend: unexpected operation '" + inst.op + "'");
        }
        code << "    uint32_t v" << inst.result->value << " = " << rhs << ";\n";
        defined.insert(inst.result->value);
    }
    // Process exit code is defined only for our narrow 0..255 demonstration.
    // Do NOT confuse this backend with a general language return/ABI contract.
    code << "    return (int)(" << value(block.terminator->operands.front(), defined)
         << " & 255u);\n}\n";
    session.artifacts().insert<std::string>("c-source", code.str());
}

} // namespace novac::backend
