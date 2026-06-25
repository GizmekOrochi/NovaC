# Assets Essentials

Essentials is the standard imperative language-building package for NovaC.

It is designed to mirror the modular style of Assets Atomic. Atomic provides expression-level primitives such as literals and operators. Essentials provides structural language features such as programs, scopes, variables, control flow, return statements, and functions.

Essentials does not replace the Engine. It installs reusable AST schemas, parser rules, and runtime handlers into an existing `EngineController`.

---

# Architecture

```text
EssentialsController
├── Program
├── Scopes
│   └── ScopedBlocks
├── Variables
│   ├── VariableDeclaration
│   ├── VariableExpression
│   ├── AssignmentStatement
│   └── ExpressionStatement
├── Control Flow
│   ├── IfStatement
│   ├── WhileStatement
│   └── ForStatement
└── Functions
    ├── FunctionDeclaration
    ├── FunctionCall
    └── ReturnStatement
```

---

# Public API

```cpp
#include "novac/assets/essentials/EssentialsController.hpp"

novac::assets::essentials::EssentialsController essentials{engine};

essentials.installStandardCore();
```

Or install modules independently:

```cpp
essentials.installProgram();
essentials.installScopedBlocks();
essentials.installVariables();
essentials.installExpressionStatements();
essentials.installIfStatements();
essentials.installWhileLoops();
essentials.installForLoops();
essentials.installReturnStatements();
essentials.installFunctions();
```

---

# Standard Core

`installStandardCore()` installs:

* program root parsing
* scoped blocks
* variables
* assignments
* expression statements
* if / else statements
* while loops
* for loops
* return statements
* function declarations
* function calls
* native `print`

---

# Example

```cpp
novac::controllers::EngineController engine{};

novac::assets::atomic::AtomicController atomics{engine};
novac::assets::essentials::EssentialsController essentials{engine};

atomics.installStandardCore();
essentials.installStandardCore();

const std::string source{
R"(
func add(a, b) {
    return a + b;
}

func main() {
    x = 5;
    y = 6;

    z = add(x, y);

    print(z);

    if z > 10 {
        print("greater than ten");
    } else {
        print("ten or below");
    }

    while z < 20 {
        print(z);
        z = add(z, 1);
    }

    return add(z, z);
}
)"
};

const novac::ast::NodePtr program{engine.parse(source)};
engine.validate(*program);

const novac::runtime::Value result{engine.eval(*program)};

std::cout << "Program returned: " << result.toString() << '\n';
```

Expected behavior:

```text
11
greater than ten
11
12
13
14
15
16
17
18
19
Program returned: 40
```

---

# Notes

Essentials intentionally uses the Engine runtime environment instead of implementing a second scope system.

By default, `enforceChildTraits` is disabled to remain compatible with assets that have not adopted the shared traits yet. Once all assets agree on common traits, it can be enabled for stricter AST validation.
