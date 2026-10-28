# Historical precompilation tests (not built)

These tests refer to removed `novac/assets/compilation/ir/*` headers, which
were moved into Engine. Their active equivalents are in
`tests/engine/transformation/` (`generic_ir_tests.cpp`, `ir_lifetime_tests.cpp`,
`ir_validation_tests.cpp`, `hir_mir_regression_tests.cpp`). They are retained
as reference text, not executable `.cpp` tests. No obsolete Compilation IR was
reintroduced to make them compile.
