# Backend submodule of Compilation (parked / not linked)

This directory holds the former backend interface and registry, moved out of the
engine so the core stays independent of compilation or code generation.

**Temporary constraint:** the current `Backend::run` takes
`compilation::CompilationSession&`, defined in the parent compilation
asset. This is intentionally preserved to avoid a premature API redesign, not a
long-term contract. When compilation orchestration is redesigned, the backend
context can be decoupled from that session.

The asset is not included in `<NovaC.hpp>`, not linked in `libNovaC.a`, and not
installed. The backend registry is not automatically registered or invoked.

The unified compilation asset is parked for later work; it is not currently a supported
standalone public API. The Engine retains its source/lexer/parser/AST/runtime and generic IR primitives,
but has no dependency on the compilation asset or its backend.
