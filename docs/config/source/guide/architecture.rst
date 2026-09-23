Architecture
============

NovaC separates **infrastructure** from **language semantics**.

The Engine knows how to tokenize, dispatch parser rules, validate generic AST
schemas, dispatch runtime handlers, and run lowering passes. It does not decide
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
        |
        | optional lowering
        v
       HIR
        |
        | optional lowering
        v
       MIR
        |
        v
   your backend / optimizer / analysis

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

   source -> tokens -> AST -> HIR -> MIR -> your next stage

HIR/MIR are optional tools for projects that need normalized intermediate
representations. They are explained from first principles in :doc:`ir`.

Registries are the extension mechanism
--------------------------------------

NovaC avoids one giant hardcoded grammar or visitor. Most behavior is installed
into registries:

* lexer keywords, symbols, comments, and identifier rules;
* parser rules grouped by domains;
* AST node schemas;
* runtime handlers by node kind;
* AST-to-HIR lowerers;
* HIR-to-MIR lowerers;
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

The upper layers do not replace the Engine. Atomic and Essentials configure it;
Types can coexist with it without becoming part of the engine core.

Choose the smallest layer you need
----------------------------------

A calculator may use only Engine + Atomic. A dynamically typed scripting language
may use Engine + Atomic + Essentials without Types, while a statically typed
language can add TypeController explicitly. A compiler experiment may use Engine and custom lowering rules
without Essentials at all.

That ability to stop at the level you need is a central design goal of NovaC.
