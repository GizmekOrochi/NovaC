Architecture
============

NovaC separates **infrastructure** from **language semantics**.

The Engine knows how to tokenize, dispatch parser rules, validate generic AST
schemas, and dispatch runtime handlers. It does not decide
that your language must have ``if``, ``+``, functions, or even executable
semantics.

The complete picture
--------------------

.. code-block:: text

   source text
       |
       v
   +---------+
   |  Lexer  |  rules: keywords, symbols, identifiers, comments
   +----+----+
        |
        v
      tokens
        |
        v
   +---------+
   | Parser  |  domains + prefix/infix/postfix/fallback rules
   +----+----+
        |
        v
       AST  ---------------------------> Runtime evaluation

HIR/MIR/IR are optional **Engine infrastructure**, available for both
interpreters and analysis. Compiler sessions, pass orchestration and backends
remain together in the parked ``novac/assets/compilation`` asset (backend
under ``compilation/backend``) and are not part of the installed library.

There are therefore two common NovaC workflows.

Interpreter workflow
^^^^^^^^^^^^^^^^^^^^

.. code-block:: text

   source -> tokens -> AST -> Runtime

This is what Atomic + Essentials use in the examples. It is enough for a fully
working interpreted language.

Compiler / analysis workflow
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: text

   source -> tokens -> AST -> Engine HIR/MIR -> your interpreter or analyzer

HIR/MIR live in Engine. A separate compilation asset may consume them later.
See :doc:`ir` for the distinction and future integration boundary.

Registries are the extension mechanism
--------------------------------------

NovaC avoids one giant hardcoded grammar or visitor. Most behavior is installed
into registries:

* lexer keywords, symbols, comments, and identifier rules;
* parser rules grouped by domains;
* AST node schemas;
* runtime handlers by node kind;
* generic type definitions, type-to-type conversions, operation signatures, and semantic rules.

This is why independent language features can be composed without editing the
Engine.

Controllers are convenience layers
----------------------------------

``EngineController``
   Low-level façade over the registries and processing pipeline.

``AtomicController``
   Packages common expression features: literals and operators.

``EssentialsController``
   Packages common imperative features: program roots, scopes, variables,
   functions, returns, conditionals, and loops.

``TypeController``
   Independent optional controller for generic language-defined types, aliases,
   conversions, and typing semantics attached to Atomic literal/operation
   features. It owns its own validation/finalization lifecycle and is not stored
   inside ``EngineController``.

``MemoryController``
   Independent optional bit-precise memory asset layered over Types/Layout/Storage.
   It tracks address spaces, regions, allocations, lifetimes, and typed references
   while raw address-space behavior remains pluggable through ``BitAccess``.

The upper layers do not replace the Engine. Atomic and Essentials configure it;
Types can coexist with it without becoming part of the engine core.

Choose the smallest layer you need
----------------------------------

A calculator may use only Engine + Atomic. A dynamically typed scripting language
may use Engine + Atomic + Essentials without Types, while a statically typed
language can add TypeController explicitly. Interpreters and compiler experiments can use the IR primitives
independently of compiler orchestration.

That ability to stop at the level you need is a central design goal of NovaC.
