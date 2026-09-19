# Assets Atomic

Atomic is the standard language-building package provided by NovaC. It contains reusable language primitives that can be installed directly into an Engine instance.
Unlike the Engine, which only provides infrastructure, Atomic provides actual language constructs such as literals and operators. Its purpose is to accelerate language development by supplying common building blocks that most languages require.

It is built around composable language features. Each feature installs itself into the Engine through the AtomicController.

Features may register:

* lexer patterns
* parser rules
* AST schemas
* runtime handlers

This allows language primitives to remain independent and reusable.

---

# Architecture Overview

```text
                AtomicController
                       │
       ┌───────────────┼───────────────┐
       ▼                               ▼
    Literals                      Operations
       │                               │
 ┌─────┼─────┐               ┌─────────┼─────────┐
 ▼     ▼     ▼               ▼         ▼         ▼
Int  Float String         Numeric Comparison Logical
              ▼
           Boolean
```

---

# Atomic Controller

The AtomicController is the entry point of the Atomic module. It acts as an installation and integration layer between Atomic features and the Engine.
AtomicController itself does not define language constructs. Instead, the language features are provided through reusable LiteralPack and OperationPack objects which can be installed into the controller.

Responsibilities include:

* feature installation
* feature ownership
* pattern registration
* AST bootstrap
* runtime integration
* expression node registration
* feature validation

The controller validates feature metadata before installation and prevents duplicate registrations.

---

# Literals

Atomic provides ready-to-use literal implementations.

Currently supported literals:

* Integer literals
* Floating-point literals
* String literals
* Boolean literals

Each literal installs:

* token recognition patterns
* AST node schemas
* parser integration
* runtime evaluation

Literal implementations may also support suffix matching and suffix validation.

Examples:

```text
42
3.14
"hello"
true
false
```

Implemented through:

* IntegerLiteralAtomic
* FloatLiteralAtomic
* StringLiteralAtomic
* BooleanLiteralAtomic

---

# Numeric Operations

Atomic provides arithmetic operators.

Supported operations include:

```text
+
-
*
/
%
unary -
```

Numeric operations automatically install:

* Pratt parser operators
* AST generation
* runtime execution handlers

Operator precedence and associativity are handled automatically.

## Integer overflow semantics

Atomic integer arithmetic uses checked semantics. Integer addition, subtraction,
multiplication, division, and unary negation never silently wrap and never rely
on signed C++ overflow. If the mathematical result cannot be represented by the
runtime `int` type, evaluation throws `std::runtime_error`. Integer division and
modulo by zero also throw. `INT_MIN % -1` is defined explicitly as `0`, avoiding
the undefined C++ remainder expression while preserving the mathematical
remainder. Floating-point arithmetic is unchanged.

---

# Comparison Operations

Atomic provides comparison operators.

Supported operations include:

```text
==
!=
<
<=
>
>=
```

Comparison operators evaluate to boolean values and integrate directly with the runtime.

---

# Logical Operations

Atomic provides logical operators.

Supported operations include:

```text
&&
||
!
```

Logical operators use runtime truthiness semantics and can be composed with all other atomic expressions.

---

# Standard Language Core

Atomic can bootstrap a minimal expression language through a single installation step.

The standard core includes:

* all atomic literals
* numeric operators
* comparison operators
* logical operators

This provides a ready-to-use expression language suitable for calculators, scripting languages, DSLs, and language prototypes.

---

# Extensibility

Atomic is not limited to the built-in features.

Users may create custom:

* literals
* operators
* token patterns
* runtime handlers

and install them through the same AtomicController used by the built-in components.

This allows Atomic to act as a foundation layer for larger language ecosystems.

---

# Example

```cpp
#include <iostream>
#include <string>

#include "novac/assets/atomic/AtomicController.hpp"
#include "novac/engine/EngineController.hpp"

namespace {

void printExpression(const novac::controllers::EngineController &engine, const std::string &source) {
    const novac::ast::NodePtr root{engine.parse(source)};

    engine.validate(*root);

    const novac::runtime::Value value{engine.eval(*root)};

    std::cout << source << " => " << value.toString() << '\n';
}

} // namespace

int main() {
    using namespace novac::assets::atomic;

    novac::controllers::EngineController engine{};
    AtomicController atomics{engine};

    atomics.use(literals::integer(
            "IntegerLiteral",
            {"int", "i32", "i64"}
        )
    );

    atomics.use(literals::floating(
            "FloatLiteral",
            {"float", "f32", "f64"}
        )
    );

    atomics.use(literals::stringLiteral(
            "StringLiteral",
            {"string", "str", "char"}
        )
    );

    atomics.use(literals::boolean(
            "BooleanLiteral",
            "yes",
            "no"
        )
    );

    atomics.use(operations::numeric({
            .add = "plus",
            .subtract = "minus",
            .multiply = "mul",
            .divide = "div",
            .modulo = "mod",
            .negate = "neg"
        })
    );

    atomics.use(operations::comparison({
            .equal = "is",
            .lessEqual = "at_most",
            .greater = "above"
        })
    );

    atomics.use(operations::logical({
            .andToken = "and",
            .orToken = "or",
            .notToken = "not"
        })
    );

    std::cout << "== atomic expressions with custom tokens ==\n";

    printExpression(engine, "10_i32 plus 20_int mul 3_i64");
    printExpression(engine, "3.5_f32 plus 2.25_float");
    printExpression(engine, "neg 10_int plus 4_i32");
    printExpression(engine, "10_i64 above 3_int and yes");
    printExpression(engine, "not no or no");
    printExpression(engine, "10_int at_most 10_i32");
    printExpression(engine, "42_i64 is 42_int");
    printExpression(engine, "\"nova\"_str");

    return 0;
}
```

Expected output:

```text
== atomic expressions with custom tokens ==

10_i32 plus 20_int mul 3_i64 => 70
3.5_f32 plus 2.25_float => 5.750000
neg 10_int plus 4_i32 => -6
10_i64 above 3_int and yes => true
not no or no => true
10_int at_most 10_i32 => true
42_i64 is 42_int => true
"nova"_str => nova
```

This example illustrates:

* Custom integer suffixes (`_int`, `_i32`, `_i64`)
* Custom floating-point suffixes (`_float`, `_f32`, `_f64`)
* Custom string suffixes (`_str`, `_char`)
* Custom boolean keywords (`yes`, `no`)
* Arithmetic expressions
* Comparison expressions
* Logical expressions
* Unary operators
* Automatic AST generation
* Automatic runtime evaluation

All parsing, AST construction, and execution behavior is installed through the AtomicController.