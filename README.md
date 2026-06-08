# NovaC

NovaC is a framework for building programming languages.

Unlike traditional compiler frameworks, NovaC does not impose a language architecture, runtime model, compilation pipeline, or backend strategy. Instead, it provides a collection of reusable components that can be assembled to create languages with widely different designs and execution models.

NovaC is designed around a simple principle:

> The framework provides infrastructure. The language provides semantics.

Whether you are building an interpreted scripting language, a statically typed systems language, a bytecode VM, a transpiler, or an experimental research language, NovaC aims to stay out of the way while providing the tools needed to build it.

---

# Philosophy

NovaC is not a language.

NovaC is not a compiler.

NovaC is a language construction framework.

The framework should know as little as possible about the language being built. Language-specific concepts belong inside the language definition, not inside the framework itself.

For example, concepts such as:

* Functions
* Classes
* Variables
* Blocks
* Conditionals
* Loops
* Return statements
* Program entry points

are language features.

NovaC does not assume that these concepts exist, nor does it provide built-in implementations for them.

Instead, NovaC focuses exclusively on providing the infrastructure required to define and process them.

---

# Core Components

## Lexer

A registry-driven tokenization system.

Languages define:

* Keywords
* Symbols
* Lexical behavior

---

## AST

A runtime-configurable abstract syntax tree system.

Languages define:

* Node kinds
* Node fields
* Validation rules

---

## Parser

A domain-based parsing framework.

Languages define:

* Expression domains
* Statement domains
* Declaration domains
* Custom parsing domains

This allows parsing behavior to be customized without hardcoding language constructs into the framework.

---

## Runtime

A registry-driven execution system.

Languages define:

* Evaluation rules
* Execution semantics
* Runtime behavior

---

## Type System

Optional infrastructure for static and dynamic typing.

Languages define:

* Types
* Type relationships
* Type checking rules
* Type inference behavior

---

## Traits

A capability and constraint system that can be used to express reusable behavior and generic requirements.

---

## Templates

Infrastructure for generic programming and compile-time specialization.

---

## Macros

Compile-time AST transformation and code generation facilities.

---

## Modules

Infrastructure for organizing code across compilation units through imports and exports.

---

## Diagnostics

Framework-wide reporting for:

* Errors
* Warnings
* Notes
* Source locations

---

## Pass Framework

Compilation is organized around passes.

NovaC does not require a specific compilation pipeline. Languages may define any sequence of passes appropriate for their architecture.

---

## Artifacts

Passes communicate through named artifacts stored inside the compilation context.

This allows compiler stages to remain loosely coupled and reusable.

---

# Architecture

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
├── Source
├── Artifacts
└── Diagnostics
```

---

# Design Goals

NovaC is built around the following goals:

* Language Agnostic
* Pass Driven
* Registry Driven
* Extensible
* Runtime Configurable
* Minimal Assumptions
* Reusable Infrastructure
* No Hardcoded Language Semantics

The framework should make building a language easier without forcing language authors into a predefined architecture.

---

# Core Assumptions

NovaC intentionally assumes very little.

The framework only assumes the existence of:

* Source code
* Tokens
* AST nodes
* Passes
* Artifacts

Everything else belongs to the language being built.

If a language requires functions, classes, modules, ownership systems, garbage collection, bytecode, JIT compilation, or entirely different concepts, those features should be implemented by the language rather than embedded into the framework.

---

# Current Status

NovaC currently provides:

* Lexer Infrastructure
* AST Registry
* Parser Domains
* Runtime Registry
* Type System
* Traits
* Templates
* Macros
* Module Infrastructure
* Diagnostics
* Pass Framework
* Artifact System

Current development is primarily focused on framework architecture, extensibility, and infrastructure rather than the implementation of a specific language.

The long-term goal is to provide a robust foundation for building a wide variety of programming languages while remaining independent of any particular language design.
