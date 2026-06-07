# NovaC User Guide

## Introduction

NovaC is a framework for creating programming languages.

Instead of providing a single language, NovaC provides the infrastructure needed to build one:

* Lexer
* AST
* Parser
* Runtime
* Type System
* Traits
* Templates
* Macros
* Modules
* Lowering
* Backends
* Compiler Passes

A language is simply a configured `Language` instance.

---

# Your First Language

Let's build a tiny language that supports:

```text
40 + 2 + 8
```

and evaluates to:

```text
50
```

---

# Step 1: Create a Language

```cpp
language::Language language{};
```

Everything will be registered into this object.

---

# Step 2: Register Tokens

Tell the lexer which symbols exist.

```cpp
language.lexer.symbol("+");
```

Now the lexer can recognize:

```text
+
```

---

# Step 3: Define AST Nodes

An AST node is declared at runtime.

## Integer

```cpp
language.nodes.registerNode({
    "integer",
    {
        {"value", ast::FieldKind::Int}
    }
});
```

## Binary Expression

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

## Program

```cpp
language.nodes.registerNode({
    "program",
    {
        {"expression", ast::FieldKind::Node}
    }
});
```

---

# Step 4: Define Parsing Rules

NovaC uses parser domains.

We create an expression domain named:

```text
expr
```

## Integer Parsing

```cpp
language.parser.prefix(
    "expr",
    "$int",
    [](parser::ParserContext& ctx)
    {
        auto token{
            ctx.consumeKind(token::Kind::Integer)
        };

        auto node{
            ast::Node::make("integer")
        };

        node->set(
            "value",
            std::stoi(token.text));

        return node;
    });
```

---

## Addition Operator

```cpp
language.parser.infix(
    "expr",
    "+",
    10,
    [](parser::ParserContext& ctx,
       ast::NodePtr left)
    {
        ctx.consume("+");

        auto right{
            ctx.parse("expr", 11)
        };

        auto node{
            ast::Node::make("binary")
        };

        node->set("left", left);
        node->set("right", right);
        node->set("op", std::string{"+"});

        return node;
    });
```

---

## Program Rule

```cpp
language.parser.fallback(
    "program",
    [](parser::ParserContext& ctx)
    {
        auto expr{
            ctx.parse("expr")
        };

        auto program{
            ast::Node::make("program")
        };

        program->set(
            "expression",
            expr);

        return program;
    });
```

---

# Step 5: Runtime Evaluation

## Integer Evaluation

```cpp
language.runtime.expression(
    "integer",
    [](const ast::Node& node,
       const runtime::RuntimeContext&)
    {
        return runtime::Value::integer(
            node.integer("value"));
    });
```

---

## Addition Evaluation

```cpp
language.runtime.binaryOperator(
    "+",
    [](const ast::Node& node,
       const runtime::RuntimeContext& ctx)
    {
        int lhs{
            ctx.eval(*node.child("left")).asInt()
        };

        int rhs{
            ctx.eval(*node.child("right")).asInt()
        };

        return runtime::Value::integer(
            lhs + rhs);
    });
```

---

## Program Evaluation

```cpp
language.runtime.expression(
    "program",
    [](const ast::Node& node,
       const runtime::RuntimeContext& ctx)
    {
        return ctx.eval(
            *node.child("expression"));
    });
```

---

# Step 6: Define Lowering

NovaC allows arbitrary lowering pipelines.

Example:

```text
AST
 ↓
HIR
 ↓
MIR
```

---

## Integer → HIR

```cpp
language.lowering.hir(
    "integer",
    [](const ast::Node& node,
       ir::HIRBuilder& builder,
       const ir::LoweringRegistry&)
    {
        builder.emit(
            "hir.const",
            {
                std::to_string(
                    node.integer("value"))
            });
    });
```

---

## Binary → HIR

```cpp
language.lowering.hir(
    "binary",
    [](const ast::Node& node,
       ir::HIRBuilder& builder,
       const ir::LoweringRegistry& registry)
    {
        registry.lowerHIR(
            *node.child("left"),
            builder);

        registry.lowerHIR(
            *node.child("right"),
            builder);

        builder.emit("hir.add");
    });
```

---

# Step 7: Create a Backend

```cpp
class DumpBackend final
    : public backend::Backend
{
public:

    std::string name() const override
    {
        return "dump";
    }

    void emit(
        const ir::MIRModule& module) override
    {
        for (const auto& node : module.nodes) {
            std::cout << node.op << '\n';
        }
    }
};
```

Register it:

```cpp
language.backends.add(
    "dump",
    [] {
        return std::make_unique<DumpBackend>();
    });
```

---

# Step 8: Build a Compiler Pipeline

NovaC compilers are pass-based.

```cpp
compiler::Compiler compiler{
    language,
    "program"
};
```

Register passes:

```cpp
compiler.addPass<compiler::ParsePass>("ast");
compiler.addPass<compiler::AstValidationPass>("ast");
compiler.addPass<compiler::HIRLoweringPass>("ast", "hir");
compiler.addPass<compiler::MIRLoweringPass>("hir", "mir");
compiler.addPass<compiler::RuntimePass>("ast", "result");
```

---

# Step 9: Run Code

```cpp
auto context{
    compiler.run("40 + 2 + 8")
};
```

Retrieve the result:

```cpp
const auto& result{
    context.requireArtifact<runtime::Value>(
        "result")
};

std::cout
    << result.toString()
    << '\n';
```

Output:

```text
50
```

---

# Understanding Artifacts

Passes communicate through artifacts.

```cpp
context.setArtifact(
    "ast",
    astRoot);
```

Later:

```cpp
auto& ast{
    context.requireArtifact<ast::NodePtr>(
        "ast")
};
```

Artifacts replace hardcoded compiler stages.

---

# Designing Your Own Pipeline

NovaC does not require:

```text
AST -> HIR -> MIR
```

You can do:

```text
AST -> Bytecode
```

or

```text
AST -> AST -> AST
```

or

```text
Source -> Interpreter
```

The framework does not enforce a compilation model.

---

# Framework Philosophy

If the framework assumes a language feature, it is probably wrong.

Examples of concepts that should NOT exist in NovaC core:

* main
* function
* variable
* block
* class
* if
* while
* for
* return

These belong to languages.

NovaC provides infrastructure.

Languages provide semantics.
