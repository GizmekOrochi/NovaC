# NovaC

NovaC is a language construction framework.

Unlike traditional compiler frameworks, NovaC does not impose a language architecture, runtime model, compiler pipeline, or backend strategy.

NovaC provides reusable infrastructure while allowing each language to define its own semantics.

---

# Philosophy

NovaC is not a language.

NovaC is not a compiler.

NovaC is a framework for building languages.

The framework should know as little as possible about the language being built.

If a concept belongs to a language, it should not exist in the framework.

Examples:

* function
* class
* variable
* block
* if
* while
* for
* return
* main

These are language concepts.

NovaC only provides infrastructure.

---

# Core Components

## Lexer

Registry-driven tokenization.

Languages define:

* keywords
* symbols

## AST

Runtime-defined node schemas.

Languages define:

* node kinds
* fields
* validation rules

## Parser

Domain-based parser system.

Languages define:

* expression domains
* statement domains
* declaration domains
* custom domains

## Runtime

Registry-driven evaluation.

Languages define execution semantics.

## Type System

Optional type infrastructure.

Languages define typing rules.

## Traits

Constraint and capability system.

## Templates

Generic programming infrastructure.

## Macros

Compile-time AST transformation.

## Modules

Import/export infrastructure.

## Diagnostics

Framework-wide error reporting.

## Passes

Compilation is pass-driven.

NovaC does not impose a compilation pipeline.

## Artifacts

Passes communicate through named artifacts.

---

# Architecture

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
+-- Source
+-- Artifacts
+-- Diagnostics

---

# Design Goals

* Language Agnostic
* Pass Driven
* Registry Driven
* Extensible
* Runtime Configurable
* Minimal Assumptions
* No Hardcoded Language Semantics

---

# Core Assumptions

NovaC assumes:

* Source exists
* Tokens exist
* AST nodes exist
* Passes exist
* Artifacts exist

Everything else belongs to the language.

---

# Current Status

NovaC currently provides:

* Lexer
* AST Registry
* Parser Domains
* Runtime Registry
* Type System
* Traits
* Templates
* Macros
* Modules
* Diagnostics
* Pass Framework
* Artifact System

NovaC is currently focused on framework architecture rather than language implementation.
