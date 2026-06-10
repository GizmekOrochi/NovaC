# NovaC

NovaC is a framework for building programming languages.

Unlike traditional compiler frameworks, NovaC does not impose a language architecture, runtime model, compilation pipeline, intermediate representation, or backend strategy.

Instead, it provides a collection of reusable infrastructure components that can be assembled to create languages with widely different designs and execution models.

NovaC is designed around a simple principle:

> The framework provides infrastructure. The language provides semantics.

Whether you are building:

* an interpreted scripting language
* a statically typed systems language
* a bytecode virtual machine
* a transpiler
* a compiler
* a research language
* a domain specific language

NovaC aims to remain language-agnostic while providing the building blocks required to implement them.

---

# Philosophy

NovaC is not a language.

NovaC is not a compiler.

NovaC is a language construction framework.

Language-specific concepts belong inside the language definition rather than inside the framework itself.

For example:

* Functions
* Classes
* Variables
* Blocks
* Loops
* Pattern Matching
* Ownership Systems
* Coroutines
* Garbage Collection
* Program Entry Points

are language features.

NovaC does not assume these concepts exist.

Instead, NovaC provides the infrastructure required to define, analyze, transform, execute, and compile them.

---

# Core Components

## Lexer

Registry-driven tokenization.

Languages define:

* Keywords
* Symbols
* Lexical rules

---

## AST

Runtime-configurable abstract syntax tree infrastructure.

Languages define:

* Node kinds
* Field schemas
* Validation rules

---

## Parser

Domain-based parsing infrastructure.

Languages define:

* Expression domains
* Statement domains
* Declaration domains
* Custom parsing domains

NovaC uses Pratt parsing for expression-oriented grammars while remaining extensible.

---

## Semantic Layer

Optional semantic analysis infrastructure.

Provides:

* Symbols
* Definitions
* References
* Scopes
* Semantic Contexts

Languages define:

* Name resolution
* Type checking
* Semantic rules
* Validation passes

---

## Runtime

Registry-driven interpretation infrastructure.

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

Capability and constraint infrastructure for reusable behavior and generic requirements.

---

## Templates

Infrastructure for generic programming and specialization.

---

## Macros

Compile-time AST transformation and code generation facilities.

---

## Modules

Infrastructure for organizing code across compilation units.

Provides:

* Imports
* Exports
* Resolution

---

## Diagnostics

Framework-wide reporting infrastructure.

Supports:

* Errors
* Warnings
* Notes
* Source Locations

---

## Intermediate Representations

NovaC provides optional IR infrastructure.

### HIR

High-Level Intermediate Representation.

Provides:

* Typed Values
* Instructions
* Basic Blocks
* Control Flow Foundation

### MIR

Mid-Level Intermediate Representation.

Provides:

* Typed Values
* Instructions
* Basic Blocks
* Backend-Oriented Lowering

NovaC does not impose a specific lowering pipeline.

Languages may use:

AST → HIR → MIR

or

AST → MIR

or

AST → Bytecode

or any other architecture.

---

## Pass Framework

Compilation is organized around passes.

NovaC does not require a predefined compilation pipeline.

Languages define their own pipelines.

---

## Artifact System

Passes communicate through named artifacts stored inside the compilation context.

Examples:

* AST
* Semantic Context
* HIR
* MIR
* Runtime Values
* Bytecode
* Analysis Results

This allows passes to remain loosely coupled and reusable.

---

# Architecture ( TO REFARCTOR )

include/
├── novac/
│   ├── engine/
│   │   ├── foundation/
│   │   │   ├── Diagnostic.hpp
│   │   │   ├── Ids.hpp
│   │   │   └── registry/
│   │   │
│   │   ├── syntax/
│   │   │   ├── Token.hpp
│   │   │   ├── Lexer.hpp
│   │   │   ├── Node.hpp
│   │   │   └── Parser.hpp
│   │   │
│   │   ├── semantic/
│   │   │   ├── Semantic.hpp
│   │   │   ├── TypeSystem.hpp
│   │   │   └── Overload.hpp
│   │   │
│   │   ├── execution/
│   │   │   └── Runtime.hpp
│   │   │
│   │   ├── transformation/
│   │   │   └── IR.hpp
│   │   │
│   │   └── compilation/
│   │       ├── CompilationContext.hpp
│   │       ├── Compiler.hpp
│   │       └── Pass.hpp
│   │
│   ├── assets/
│   │   ├── language/
│   │   │   ├── Language.hpp
│   │   │   ├── LanguageFeature.hpp
│   │   │   └── LanguageBuilder.hpp
│   │   │
│   │   ├── modules/
│   │   │   └── Module.hpp
│   │   │
│   │   ├── generics/
│   │   │   ├── Templates.hpp
│   │   │   └── Traits.hpp
│   │   │
│   │   ├── macros/
│   │   │   └── Macro.hpp
│   │   │
│   │   ├── passes/
│   │   │   ├── ParsePass.hpp
│   │   │   ├── SemanticPass.hpp
│   │   │   ├── LoweringPass.hpp
│   │   │   └── RuntimePass.hpp
│   │   │
│   │   └── standard/
│   │       ├── StandardTokens.hpp
│   │       ├── StandardNodes.hpp
│   │       ├── StandardTypes.hpp
│   │       └── StandardRuntime.hpp
│   │
│   └── controller/
│       ├── EngineController.hpp
│       ├── AssetController.hpp
│       └── NovaController.hpp 

---

# Feature System

NovaC includes an optional feature system.

Features may contribute:

* Tokens
* AST Nodes
* Parsing Rules
* Runtime Behavior
* Semantic Rules
* Types
* Traits
* Templates
* Macros
* Lowering Rules

Features can declare:

* Dependencies
* Capabilities
* Conflicts
* Version Requirements

This allows languages to be assembled from reusable building blocks.

---

# Design Goals

NovaC is built around the following goals:

* Language Agnostic
* Pass Driven
* Registry Driven
* Feature Driven
* Extensible
* Runtime Configurable
* Minimal Assumptions
* Reusable Infrastructure
* No Hardcoded Language Semantics

The framework should make building languages easier without forcing language authors into a predefined architecture.

---

# Core Assumptions

NovaC intentionally assumes very little.

The framework only assumes the existence of:

* Source Code
* Tokens
* AST Nodes
* Passes
* Artifacts

Everything else belongs to the language being built.

If a language requires:

* Classes
* Ownership
* Bytecode
* JIT Compilation
* Garbage Collection
* Borrow Checking
* Coroutines
* Pattern Matching

those concepts should be implemented by the language itself.

---

# Current Status

NovaC currently provides:

* Lexer Infrastructure
* AST Infrastructure
* Parser Infrastructure
* Semantic Infrastructure
* Runtime Infrastructure
* Type System
* Traits
* Templates
* Macros
* Module Infrastructure
* Diagnostics
* Typed HIR
* Typed MIR
* Pass Framework
* Artifact System
* Feature System

Current development focuses on framework architecture, scalability, extensibility, and language-building infrastructure rather than the implementation of a specific language.

The long-term goal is to provide a robust foundation for building a wide variety of programming languages while remaining independent of any particular language design.
