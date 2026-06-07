# Implementation Depth Checklist

## Done in V8

- Runtime dispatch through `RuntimeRegistry`
- Feature-owned runtime logic
- Variables and assignments
- Function parameters and calls
- If/while runtime handlers
- Generic instantiation cache smoke test
- Operator overload ambiguity detection
- AST -> HIR -> MIR smoke pipeline

## Next depth targets

1. Real source-mapped macro hygiene
2. Module resolver with cache and circular import detection
3. Full template body specialization with substitution
4. Function overload resolution
5. String/float/array/struct runtime semantics
6. Multi-error diagnostic recovery
7. Real tests per feature/registry
