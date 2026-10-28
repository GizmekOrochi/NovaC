#pragma once

#include "novac/assets/compilation/ModularCompilationController.hpp"

namespace novac::assets::atomic::compilation {
/** Optional portable contribution: UnaryExpr/increment -> MIR operation.
 * A language must also provide semantic resolution for "atomic.increment.i32"
 * and the lowering of its operand. This pack is not linked by default. */
void installIncrementLowering(novac::compilation::ModularCompilationController &controller);
} // namespace novac::assets::atomic::compilation
