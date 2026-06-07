# V11 depth fixes

## LoweringRegistry

Lowering is now fully registry-driven:

```cpp
l.hir("return", ...);
l.mir("hir.return", ...);
```

`ASTLoweringPass` no longer contains a legacy `if node.kind()` lowering chain. Missing lowerers now produce an explicit error.

## Type conversions

`TypeRegistry` contains `ConversionRegistry` and `assignable()` supports:

- exact types
- unknown types
- type variables via `TypeUnifier`
- generic type argument assignability
- implicit conversion lookup

## Templates

`InstantiationEngine` now creates:

- substitution map
- deep AST clone
- mangled specialization name
- specialization cache entry
- `SpecializationRegistry` symbol

## Traits

`TraitRegistry` now supports:

- trait methods
- associated types
- implementation records
- missing method validation
- missing associated type validation

## Modules

`ImportResolver` now includes:

- module cache
- circular import detection
- manual exports
- public/private visibility
- namespace symbol table
- qualified lookup

## Macros

Macro expansion tracks:

- definition span
- call site span
- token mapping records
- hygienic generated names
- recursion limit
