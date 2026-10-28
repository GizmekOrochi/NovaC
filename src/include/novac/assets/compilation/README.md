# Compilation asset (parked / not linked)

The former Engine compilation layer is preserved here to keep it available
for a future redesign, **without integrating it into NovaC's core**.

It currently contains:

- `Compilation.hpp` / `Compilation.cpp`: artifact storage, sessions and pass scheduling.
- `CompilationController.hpp` / `CompilationController.cpp`: legacy optional compilation orchestration.
- `backend/Backend.hpp` / `backend/Backend.cpp`: the original backend interface and registry, kept as a submodule of this same asset.

## Isolation status

- None of these source files is included in `libNovaC.a` (`Makefile` excludes the entire compilation asset, including its backend subdirectory).
- The original `novac/engine/compilation`, `novac/engine/backend` and separate `novac/assets/backend` directories are removed.
- These parked headers are **not installed** as part of NovaC's public package.
- `<NovaC.hpp>` does **not** include the asset headers.
- `EngineController` does **not** install or instantiate this asset.
- No automatic registration, target, dependency or link step is provided.
- Its former tests are preserved in `tests/assets/compilation/`, outside the default test target.
- `make check-compilation-isolation` checks sources and exported/undefined archive symbols; package smoke also checks the installed headers.

This is **parked code, not a supported/usable compiler feature** in the current
build. The implementation has intentionally been preserved close to its
previous state and can be replaced/reworked later. Existing namespace names
remain for now, but are not a stability promise.

The Engine retains **generic HIR/MIR/IR**, builders, validators and lowering
primitives in `novac/engine/transformation/`, available for interpreters and
other execution models. No IR implementation is duplicated in this asset.
`CompilationSteps` owns a lowering registry within this parked asset; it does not
access lowering through `EngineController`.

The original `novac::backend` C++ namespace is deliberately unchanged in this
move; naming and interfaces can be refactored when compilation is reactivated.
