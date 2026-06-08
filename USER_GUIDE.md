# NovaC User Guide

## Introduction

NovaC is a framework for building programming languages.

A language is created by configuring a Language object.

---

# Minimal Language

A minimal language requires:

* Tokens
* AST Nodes
* Parser Rules
* Runtime Rules

---

# Create a Language

```cpp
language::Language language{};
```

---

# Register Tokens

```cpp
language.lexer.symbol("+");
```

---

# Register AST Nodes

```cpp
language.nodes.registerNode({
    "integer",
    {
        {"value", ast::FieldKind::Int}
    }
});
```

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

# Register Parser Rules

```cpp
language.parser.prefix(
    "expr",
    "$int",
    parseInteger);
```

```cpp
language.parser.infix(
    "expr",
    "+",
    10,
    parseAddition);
```

---

# Register Runtime Rules

```cpp
language.runtime.expression(
    "integer",
    evaluateInteger);
```

```cpp
language.runtime.binaryOperator(
    "+",
    evaluateAddition);
```

---

# Create a Compiler

```cpp
compiler::Compiler compiler{
    language,
    "program"
};
```

---

# Create a Pipeline

## Direct Interpretation

```cpp
compiler.addPass<compiler::ParsePass>(
    "ast");

compiler.addPass<compiler::RuntimePass>(
    "ast",
    "result");
```

---

## AST → HIR → MIR

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

---

# Run

```cpp
auto context{
    compiler.run(
        "40 + 2 + 8")
};
```

Retrieve result:

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

Example:

```cpp
context.setArtifact(
    "ast",
    ast);
```

Retrieve:

```cpp
auto &ast{
    context.requireArtifact<
        ast::NodePtr>(
            "ast")
};
```

Artifacts replace hardcoded compiler stages.

---

# Designing Pipelines

NovaC does not require:

AST → HIR → MIR

You may implement:

Source → Runtime

AST → Bytecode

AST → AST → AST

Source → Custom VM

The framework does not impose a compilation model.
