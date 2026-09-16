Quick start
===========

This page builds the smallest complete imperative language using NovaC's
standard assets.

The six steps
-------------

1. Create an Engine.
2. Install standard Atomic expressions.
3. Install standard Essentials statements/functions.
4. Parse source text.
5. Validate the resulting AST.
6. Evaluate the AST.

.. code-block:: cpp

   #include <iostream>
   #include <string>

   #include "NovaC.hpp"

   int main() {
       novac::controllers::EngineController engine{};

       novac::assets::atomic::AtomicController atomic{engine};
       atomic.installStandardCore();

       novac::assets::essentials::EssentialsController essentials{engine};
       essentials.installStandardCore();

       const std::string source{R"(
   func main() {
       let x = 20;
       let y = 22;
       return x + y;
   }
   )"};

       const auto program{engine.parse(source)};
       engine.validate(*program);

       const auto result{engine.eval(*program)};
       std::cout << result.toString() << '\n';
   }

Expected output:

.. code-block:: text

   42

What just happened?
-------------------

``EngineController``
   Owns the language infrastructure: lexer registry, parser registry, AST
   schemas, runtime dispatch, diagnostics, and optional lowering infrastructure.

``AtomicController``
   Installs expression-level behavior. The standard core provides integer,
   floating-point, string, and boolean literals plus common arithmetic,
   comparison, and logical operators.

``EssentialsController``
   Installs statement-level behavior: program root, scopes, variables,
   expression statements, functions, returns, conditionals, and loops.

``engine.parse(source)``
   Runs lexing and parsing and returns a generic AST.

``engine.validate(*program)``
   Checks that the generated AST respects the registered node schemas.

``engine.eval(*program)``
   Executes the tree using runtime handlers registered by the installed
   features.

The important design point
--------------------------

The Engine itself does not contain a built-in ``func`` statement, variable
semantics, or arithmetic language. Those behaviors were installed by Atomic
and Essentials.

That distinction is what makes NovaC a language framework rather than one
fixed interpreter.

Next
----

Continue with :doc:`first_language` to customize syntax and add a native
function, or :doc:`integration` if you want to compile NovaC into your own
project.
