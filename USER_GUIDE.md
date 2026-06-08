# NovaC User Guide

## Introduction

NovaC is a framework designed to make building programming languages simple and flexible.

Instead of forcing a specific compiler architecture, NovaC lets you define your language by configuring a `Language` object and assembling the compilation pipeline that best fits your needs.

At its core, a language consists of four elements:

* Tokens
* AST nodes
* Parser rules
* Runtime rules

---

# Creating a Language

Start by creating a language instance:

```cpp
language::Language language{};
```

---

# Defining Tokens

Tokens describe the symbols and lexical elements recognized by the lexer.

For example, to register the `+` operator:

```cpp
language.lexer.symbol("+");
```

---

# Defining AST Nodes

AST nodes represent the structure of your language.

An integer literal can be defined as:

```cpp
language.nodes.registerNode({
    "integer",
    {
        {"value", ast::FieldKind::Int}
    }
});
```

A binary expression node might look like:

```cpp
language.nodes.registerNode({
    "binary",
    {
        {"left", ast::FieldKind::Node},
        {"right", ast::FieldKind::Node},
        {"op", ast::FieldKind::String}
    }
});
```

---

# Defining Parser Rules

Parser rules describe how tokens are transformed into AST nodes.

Register a prefix rule for integer literals:

```cpp
language.parser.prefix(
    "expr",
    "$int",
    parseInteger);
```

Register an infix rule for addition:

```cpp
language.parser.infix(
    "expr",
    "+",
    10,
    parseAddition);
```

The precedence value (`10` in this example) controls operator binding strength.

---

# Defining Runtime Rules

Runtime rules specify how AST nodes are evaluated.

Register an evaluator for integer literals:

```cpp
language.runtime.expression(
    "integer",
    evaluateInteger);
```

Register an evaluator for the addition operator:

```cpp
language.runtime.binaryOperator(
    "+",
    evaluateAddition);
```

---

# Creating a Compiler

Once the language is configured, create a compiler instance:

```cpp
compiler::Compiler compiler{
    language,
    "program"
};
```

The second argument specifies the root grammar rule used when parsing source code.

---

# Building a Compilation Pipeline

NovaC uses pipelines composed of independent compiler passes.

## Direct Interpretation

For a simple interpreted language:

```cpp
compiler.addPass<compiler::ParsePass>(
    "ast");

compiler.addPass<compiler::RuntimePass>(
    "ast",
    "result");
```

This pipeline parses source code into an AST and immediately executes it.

---

## Multi-Stage Compilation

For more advanced compilers:

```cpp
compiler.addPass<compiler::ParsePass>(
    "ast");

compiler.addPass<compiler::HIRLoweringPass>(
    "ast",
    "hir");

compiler.addPass<compiler::MIRLoweringPass>(
    "hir",
    "mir");
```

This pipeline progressively lowers the program through multiple intermediate representations.

---

# Running the Compiler

Compile and execute source code:

```cpp
auto context{
    compiler.run(
        "40 + 2 + 8")
};
```

Retrieve the execution result:

```cpp
auto &result{
    context.requireArtifact<
        runtime::Value>(
            "result")
};
```

---

# Artifacts

Artifacts are named objects stored inside the compilation context.

Compiler passes communicate by producing and consuming artifacts rather than relying on fixed stages.

Store an artifact:

```cpp
context.setArtifact(
    "ast",
    ast);
```

Retrieve it later:

```cpp
auto &ast{
    context.requireArtifact<
        ast::NodePtr>(
            "ast")
};
```

This design keeps compiler passes loosely coupled and highly reusable.

---

# Designing Your Own Pipeline

NovaC does not enforce a particular compilation strategy.

Traditional pipelines such as:

```
AST → HIR → MIR
```

are fully supported, but they are not required.

Depending on your language, you might build pipelines such as:

```
Source → Runtime
AST → Bytecode
AST → AST → AST
Source → Custom VM
```

NovaC provides the building blocks; the architecture is entirely up to you.

Use as many intermediate representations, optimization passes, interpreters, or backends as your project requires.
