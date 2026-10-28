#include "novac/assets/atomic/compilation/AtomicCompilationPack.hpp"

namespace novac::assets::atomic::compilation {
void installIncrementLowering(novac::compilation::ModularCompilationController &controller) {
    using namespace novac;
    controller.features().attach(novac::compilation::CompilationFacet{
        "atomic.increment", "UnaryExpr", "atomic.increment.i32",
        [](const ast::Node &node, novac::compilation::FeatureLoweringContext &context) -> std::optional<ir::ValueId> {
            const auto child=context.requireValue(*node.child("operand"));
            return context.builder().emitValue("atomic.i32.increment",
                                               {ir::Operand::fromValue(child)}, context.semantic().type);
        }
    });
    controller.lowering().mir("atomic.i32.increment", [](const ir::MIRLoweringInput &in,
                                ir::MIRBuilder &builder, const ir::MIRLoweringContext &,
                                const ir::LoweringRegistry &) -> ir::MIRLoweringResult {
        return builder.emitValue(in.source.op, in.operands, in.source.type);
    });
}
} // namespace novac::assets::atomic::compilation
