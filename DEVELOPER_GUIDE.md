# NovaC Developer Guide

## Rule #1

No language semantics in framework code.

Never assume:

* program
* function
* variable
* class
* block
* if
* while
* for
* return
* main

---

## Rule #2

Prefer registries over switch statements.

Bad:

```cpp
switch(node.kind())
{
}
```

Good:

```cpp
runtime.expression(
    node.kind(),
    handler);
```

---

## Rule #3

Prefer passes over pipelines.

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

---

## Rule #4

Artifacts are the communication layer.

Bad:

```cpp
setAst(...);
setHIR(...);
setMIR(...);
```

Good:

```cpp
setArtifact(...);
```

---

## Rule #5

Support DuplicatePolicy.

Every registry should support:

```cpp
DuplicatePolicy::Error
DuplicatePolicy::Replace
DuplicatePolicy::Ignore
```

Silent overwrites are forbidden.

---

## Rule #6

Diagnostics before exceptions.

Language errors should be diagnostics.

Framework failures may throw exceptions.

---

## Rule #7

Everything should be replaceable.

If a subsystem cannot be replaced, it is probably too opinionated.

---

## Rule #8

Prefer artifacts over specialized APIs.

Bad:

```cpp
context.ast()
context.hir()
context.mir()
```

Good:

```cpp
context.requireArtifact(...)
```

---

## Rule #9

Framework components must not encode pipelines.

Bad:

AST → HIR → MIR

Good:

Pass → Artifact → Pass → Artifact

---

## Rule #10

The framework owns infrastructure.

The language owns semantics.

When adding code, always ask:

"Does this belong to the framework or to a language?"

If the answer is language, it must not be added to NovaC core.

---

# Long-Term Vision

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

The compiler should know nothing about the language.

The framework should know as little as possible about the language.

The language should define everything.
