# Engine — generic language infrastructure

The Engine is NovaC's language-agnostic kernel. It provides source resolution,
preprocessing, lexer, parser, AST, semantics, generic runtime execution and
control-flow signals, diagnostics, and extension registries.

**Engine also owns the generic IR infrastructure:**
`novac/engine/transformation/IR.hpp` and `GenericIR.hpp`, with HIR/MIR modules,
builders, validation, generic operations, and opt-in lowering rules. An interpreter
can execute AST directly or choose to execute its own HIR/MIR instructions.
No built-in instruction set or execution policy is imposed.

`EngineController` does not manage a compilation session or a backend and does
not expose `hir()`, `mir()`, `lowerToHIR()`, `lowerToMIR()` or `lowering()`.
These optional IR utilities remain accessible independently of that controller.

**Out of Engine:** compilation sessions, pass scheduling, compilation
orchestration, and backend interfaces/registries. They live together in one
parked asset, `novac/assets/compilation/` (backend under `compilation/backend/`).
The asset is unlinked, not installed, and not included by `<NovaC.hpp>`.
