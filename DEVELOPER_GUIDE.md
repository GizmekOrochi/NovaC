# NovaC Developer Guide

## Framework Philosophy

Before adding any code, ask:

> Does this belong to the framework, or to a language?

If the answer is "language", it must not be added to the framework.

Examples:

### Wrong

```cpp
Runtime::run()
{
    findMain();
}
```

Why?

Because not every language has a main function.

---

### Correct

```cpp
runtime.expression(
    "program",
    executeProgram);
```

The language decides what a program means.

---

## Rule #1: No Language Semantics in Core

The core must never assume:

* program
* function
* main
* variable
* block
* declaration
* if
* while
* for
* return
* class

Those belong to languages.

---

## Rule #2: Prefer Registries

Bad:

```cpp
switch(node.kind())
{
    ...
}
```

Good:

```cpp
runtime.expression(
    node.kind(),
    handler);
```

The framework should delegate behavior to registries.

---

## Rule #3: Prefer Passes

Bad:

```cpp
parse();
lower();
emit();
```

Good:

```cpp
compiler.addPass(...);
compiler.addPass(...);
compiler.addPass(...);
```

The framework should not dictate compilation pipelines.

---

## Rule #4: Artifacts are the Communication Layer

Passes should communicate through artifacts.

Bad:

```cpp
passB(passAResult);
```

Good:

```cpp
context.setArtifact("hir", hir);
```

---

## Rule #5: Validate Registrations

All registries should support:

```cpp
DuplicatePolicy::Error
DuplicatePolicy::Replace
DuplicatePolicy::Ignore
```

Silent overwrites are forbidden.

---

## Rule #6: Diagnostics Over Exceptions

Exceptions should represent framework failures.

Language errors should be diagnostics.

Preferred:

```cpp
context.diagnostics().error(...);
```

Avoid:

```cpp
throw std::runtime_error(...);
```

for user-facing language errors.

---

## Rule #7: Everything Should Be Replaceable

If a subsystem cannot be replaced, it is probably too opinionated.

Examples:

* Parser
* Runtime
* Lowering
* Backend
* Type system

should all be replaceable or extensible.

---

## Architecture Vision

Long-term NovaC architecture:

```txt
Language
    |
    +-- Lexer
    +-- AST
    +-- Parser
    +-- Runtime
    +-- Types
    +-- Traits
    +-- Templates
    +-- Macros
    +-- Modules
    +-- Lowering
    +-- Backends

Compiler
    |
    +-- PassManager
            |
            +-- Pass
            +-- Pass
            +-- Pass
            +-- Pass

CompilationContext
    |
    +-- Artifacts
    +-- Diagnostics
```

The compiler should know nothing about the language.

The framework should know as little as possible about the language.

The language should define everything.
