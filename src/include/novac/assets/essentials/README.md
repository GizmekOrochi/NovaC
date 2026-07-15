# Assets Essentials

Essentials is the standard statement-building package provided by NovaC.

While Atomic provides expressions such as literals and operators, Essentials provides the language constructs required to build complete imperative programming languages.

Essentials installs reusable language features on top of an existing `EngineController`. It does not replace the Engine nor Atomic; instead, it extends them with statements, scopes, variables, functions, and control flow.

Like Atomic, Essentials is fully modular. Every language construct is implemented as an independent feature that can be installed individually or combined into reusable packs.

Features may register:

- parser rules
- AST schemas
- runtime handlers
- language keywords
- language symbols

This allows complete language constructs to remain independent and reusable across multiple languages.

---

# Architecture Overview

```text
                EssentialsController
                         │
      ┌──────────────────┼──────────────────┐
      ▼                  ▼                  ▼
   Scopes            Variables         Control Flow
      │                  │                  │
      ▼                  ▼                  ▼
 Scoped Blocks   Variables / Assign   If / While / For
                         │
                         ▼
                 Expression Statements
                         │
                         ▼
                     Functions
                  /               \
         Declarations        Return Statements
```

---

# Essentials Controller

The `EssentialsController` is the entry point of the Essentials package.

Like `AtomicController`, it is an installation and integration layer rather than a language implementation.

Its responsibilities include:

- feature installation
- feature ownership
- parser registration
- AST schema registration
- runtime integration
- feature validation
- program bootstrap

The controller prevents duplicate feature installation and validates every feature before it is installed.

---

# Program

Essentials provides a reusable program root.

The standard program installs:

- Program AST node
- program parsing domain
- top-level execution
- automatic `main()` lookup
- function registration

A complete source file becomes a single AST node representing the entire program.

Example:

```text
func main() {

    print("Hello");

}
```

---

# Scoped Blocks

Scoped blocks introduce lexical scopes.

Each block automatically creates a new runtime scope.

Variables declared inside a block disappear when the block exits.

Example:

```text
{

    x = 42;

}
```

Each block is represented by a `BlockStmt` AST node.

---

# Variables

Essentials provides variable declarations, assignments, and variable lookups.

Supported constructs:

```text
let x = 5;

x = 10;

print(x);
```

Variable expressions integrate directly with Atomic expressions.

Assignments automatically update existing variables or create them depending on the runtime policy.

---

# Expression Statements

Expressions may be executed as standalone statements.

Example:

```text
foo();

print(x);

factorial(5);
```

Every expression statement evaluates the contained expression while discarding its result.

---

# Functions

Essentials provides reusable function declarations and function calls.

Supported constructs:

```text
func add(a, b) {

    return a + b;

}

result = add(5, 3);
```

Functions support:

- parameters
- return values
- local scopes
- recursion
- native functions

Unlike previous versions, Essentials **does not include a standard library**.

Native functions are registered explicitly through the `FunctionRegistry`, allowing every language to define its own runtime library.

---

# Return Statements

Return statements terminate the current function and optionally return a value.

Example:

```text
return value;

return;
```

Return propagation is automatically handled by the runtime.

---

# If Statements

Conditional execution is provided through `if` and `else`.

Example:

```text
if x > 0 {

    print("positive");

} else {

    print("negative");

}
```

Conditions evaluate any expression producing a truthy value.

---

# While Loops

Essentials provides traditional while loops.

Example:

```text
while x < 10 {

    x = x + 1;

}
```

Loop execution supports configurable iteration limits to prevent accidental infinite loops.

---

# For Loops

Traditional C-style for loops are supported.

Example:

```text
for(i = 0; i < 10; i = i + 1) {

    print(i);

}
```

A for loop consists of:

- initializer
- condition
- step
- body

The runtime automatically creates a scope for the loop.

---

# Standard Language Core

Essentials can bootstrap a complete imperative language through a single installation.

The standard core installs:

- program root
- scoped blocks
- variables
- expression statements
- functions
- return statements
- if / else
- while
- for

Together with Atomic, this provides a fully executable scripting language.

---

# Extensibility

Every language construct is implemented as an independent feature.

New language constructs can be added without modifying the controller.

Examples include:

- switch statements
- namespaces
- classes
- exceptions
- coroutines
- async/await
- pattern matching
- custom statements

Each feature installs itself into the Engine through the same `EssentialsController`.

---

# Example

```cpp
#include <iostream>
#include <string>

#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/assets/essentials/EssentialsController.hpp"
#include "novac/engine/EngineController.hpp"

int main() {

    novac::controllers::EngineController engine{};
    novac::assets::atomic::AtomicController atomics{engine};

    atomics.use(novac::assets::atomic::literals::standard());

    atomics.use(
        novac::assets::atomic::operations::numeric({
            .add = "+",
            .subtract = "-",
            .multiply = "*",
            .divide = "/",
            .modulo = "%",
            .negate = "-"
        })
    );

    atomics.use(
        novac::assets::atomic::operations::comparison({
            .equal = "==",
            .notEqual = "!=",
            .less = "<",
            .lessEqual = "<=",
            .greater = ">",
            .greaterEqual = ">="
        })
    );

    atomics.use(
        novac::assets::atomic::operations::logical({
            .andToken = "&&",
            .orToken = "||",
            .notToken = "!"
        })
    );

    novac::assets::essentials::EssentialsControllerOptions options{};

    options.functions.functionKeyword = "func";
    options.functions.returnKeyword = "return";
    options.functions.mainFunctionName = "main";

    novac::assets::essentials::EssentialsController essentials{
        engine,
        options
    };

    essentials.installStandardCore();

    essentials.functionRegistry().native(
        "print",
        [](const novac::ast::NodeList &arguments,
           novac::runtime::RuntimeContext &context)
           -> novac::runtime::Value {

            for(const auto &argument : arguments)
                std::cout << context.eval(*argument).toString();

            std::cout << '\n';

            return novac::runtime::Value::voidValue();
        }
    );

    essentials.functionRegistry().native(
        "toto",
        [](const novac::ast::NodeList &,
           novac::runtime::RuntimeContext &)
           -> novac::runtime::Value {

            std::cout << "toto\n";

            return novac::runtime::Value::voidValue();
        }
    );

    const std::string source{R"(

func add(a, b) {
    return a + b;
}

func factorial(n) {

    result = 1;

    while n > 1 {

        result = result * n;
        n = n - 1;

    }

    return result;
}

func main() {

    x = 5;
    y = 6;

    z = add(x, y);

    print("x =");
    print(x);

    print("y =");
    print(y);

    print("z =");
    print(z);

    if z > 10 {

        print("greater than ten");

    } else {

        print("ten or below");

    }

    sum = 0;

    for(i = 0; i < 5; i = i + 1) {

        sum = sum + i;

    }

    print("sum =");
    print(sum);

    print("factorial =");
    print(factorial(5));

    toto();

    return z + sum;
}

)"};

    const novac::ast::NodePtr program{engine.parse(source)};

    engine.validate(*program);

    const novac::runtime::Value result{engine.eval(*program)};

    std::cout << "Program returned: "
              << result.toString()
              << '\n';

    return 0;
}
```

Expected output:

```text
x =
5
y =
6
z =
11
greater than ten
sum =
10
factorial =
120
toto
Program returned: 21
```

This example demonstrates:

- installation of Atomic literals and operators
- installation of the Essentials imperative language
- user-defined native functions (`print`, `toto`)
- function declarations and calls
- variable assignments
- conditional statements
- while loops
- for loops
- automatic parsing
- automatic AST validation
- automatic runtime execution

The language itself remains independent from its standard library: native functions are registered explicitly through the `FunctionRegistry`.

---

# Relationship with Atomic

NovaC is organized into layered language assets.

```text
               Engine
                  │
        ┌─────────┴─────────┐
        ▼                   ▼
      Atomic          Essentials
        │                   │
        └─────────┬─────────┘
                  ▼
          Complete Language
```

- **Engine** provides parsing, AST infrastructure, validation, lowering, and runtime.
- **Atomic** provides expressions, literals, and operators.
- **Essentials** provides statements, variables, functions, scopes, and control flow.

Together they form a reusable foundation for building complete programming languages.

---

# Future Extensions

Essentials intentionally focuses on executable language constructs.

More advanced language capabilities are expected to be provided by additional assets, such as:

- Types
- Memory
- Modules
- Object-Oriented Programming
- Generics
- Pattern Matching
- Concurrency
- Reflection

This keeps Essentials lightweight while allowing NovaC to scale into a complete language framework.