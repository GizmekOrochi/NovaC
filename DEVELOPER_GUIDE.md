# NovaC Developer Guide

This document defines the architectural principles of NovaC.

Every contribution to the framework should be evaluated against these rules.

When in doubt, prefer preserving the architecture over adding features.

---

# Rule #1 — No Language Semantics in Framework Code

The framework must not assume the existence of language constructs.

Never hardcode concepts such as:

* Program
* Function
* Variable
* Class
* Block
* If
* While
* For
* Return
* Main

These concepts belong to languages, not to NovaC.

If a feature only makes sense for a particular language design, it does not belong in the framework.

---

# Rule #2 — Prefer Registries Over Switch Statements

Framework behavior should be data-driven and extensible.

Avoid dispatch based on hardcoded node kinds.

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

Registries allow languages to define behavior without modifying framework code.

---

# Rule #3 — Prefer Passes Over Pipelines

NovaC should not encode a fixed compilation model.

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

Compilation is a sequence of passes chosen by the language author.

The framework provides execution infrastructure, not compilation architecture.

---

# Rule #4 — Artifacts Are the Communication Layer

Compiler components communicate through artifacts.

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

Artifacts remove assumptions about intermediate representations and allow arbitrary compiler designs.

---

# Rule #5 — Support Duplicate Policies

Every registry should explicitly define duplicate handling.

Required policies:

```cpp
DuplicatePolicy::Error
DuplicatePolicy::Replace
DuplicatePolicy::Ignore
```

Silent overwrites are forbidden.

Ambiguous behavior eventually becomes a bug.

---

# Rule #6 — Diagnostics Before Exceptions

Language errors are diagnostics.

Framework failures are exceptions.

Examples of diagnostics:

* Syntax errors
* Type errors
* Undefined symbols
* Invalid language constructs

Examples of exceptions:

* Invalid framework state
* Registry corruption
* Internal invariants violated

Users should receive diagnostics whenever recovery is possible.

---

# Rule #7 — Everything Should Be Replaceable

Framework components should be loosely coupled.

If a subsystem cannot be replaced, extended, or bypassed, it is likely too opinionated.

NovaC should provide defaults where useful, but ownership must remain with the language.

---

# Rule #8 — Prefer Artifacts Over Specialized APIs

Avoid APIs that encode assumptions about compiler structure.

Bad:

```cpp
context.ast();
context.hir();
context.mir();
```

Good:

```cpp
context.requireArtifact(...);
```

The framework should not know which representations exist.

Only the language should decide that.

---

# Rule #9 — Framework Components Must Not Encode Pipelines

Intermediate representations are language concerns.

Bad:

```text
AST → HIR → MIR
```

Good:

```text
Pass → Artifact → Pass → Artifact
```

A language may use AST, HIR, MIR, bytecode, SSA, custom graphs, interpreters, virtual machines, or no intermediate representation at all.

NovaC must remain agnostic.

---

# Rule #10 — Infrastructure Belongs to the Framework. Semantics Belong to the Language.

Before adding any feature, ask:

> Does this belong to the framework or to a language?

If the answer is language, it must not be added to NovaC Core.

This rule takes precedence over all others.

---

# Architectural Vision

```text
Language
│
├── Lexer
├── AST
├── Parser
├── Runtime
├── Types
├── Traits
├── Templates
├── Macros
└── Modules


Compiler
│
└── PassManager
    │
    ├── Pass
    ├── Pass
    ├── Pass
    └── Pass


CompilationContext
│
├── Artifacts
└── Diagnostics
```

The compiler should know nothing about the language.

The framework should know as little as possible about the language.

The language should define everything.

---

# The NovaC Principle

Infrastructure is universal.

Semantics are language-specific.

NovaC owns the infrastructure.

The language owns the semantics.
