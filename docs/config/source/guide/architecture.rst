Architecture
============

NovaC separates **infrastructure** from **language semantics**.

The Engine knows how to resolve/preprocess logical sources, tokenize text,
dispatch parser rules, validate generic AST schemas, dispatch runtime handlers,
and run lowering passes. It does not decide that your language must have
``if``, ``+``, functions, a filesystem, or even executable semantics.

The complete picture
--------------------

NovaC has an optional source/preprocessor front-end in addition to the direct
text API:

.. code-block:: text

   source specifier                     direct source text
        |                                      |
        v                                      |
   SourceController                            |
        |                                      |
        v                                      |
      Source                                   |
        |                                      |
        v                                      |
   PreprocessorController                      |
        |                                      |
        v                                      |
   source-origin fragments                     |
        |                                      |
        +------------------+-------------------+
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

``engine.parse(text)`` deliberately follows the direct path and does not run the
preprocessor. ``engine.parseSource(...)`` uses the logical-source path. This
keeps existing embedders deterministic while making include/import and
conditional compilation opt-in.

Interpreter workflow
^^^^^^^^^^^^^^^^^^^^

Without preprocessing:

.. code-block:: text

   text -> tokens -> AST -> Runtime

With preprocessing:

.. code-block:: text

   Source -> source fragments -> tokens -> AST -> Runtime

Atomic + Essentials use the first form in most introductory examples. Both paths
converge on the same lexer/parser/runtime infrastructure.

Compiler / analysis workflow
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: text

   [Source -> preprocessing ->] tokens -> AST -> HIR -> MIR -> your next stage

HIR/MIR are optional tools for projects that need normalized intermediate
representations. They are explained from first principles in :doc:`ir`.

Registries and callbacks are the extension mechanism
----------------------------------------------------

NovaC avoids one giant hardcoded grammar or visitor. Most behavior is installed
into registries or ordered callback chains:

* source resolvers;
* preprocessing directives and named pragma hooks;
* lexer keywords, symbols, comments, and identifier rules;
* parser rules grouped by domains;
* AST node schemas;
* runtime handlers by node kind;
* runtime control-signal identifiers owned by language features;
* AST-to-HIR lowerers;
* HIR-to-MIR lowerers;
* generic type definitions, type-to-type conversions, operation signatures,
  and semantic rules.

This is why independent language features can be composed without editing the
Engine. ``EngineFeature`` installation is transactional, including registrations
made in the source/preprocessor layer.

Controllers and facilities
--------------------------

``EngineController``
   Main language-agnostic façade. It owns the core registries plus
   ``SourceController`` and ``PreprocessorController`` and exposes both direct
   text and logical-source processing paths.

``SourceController``
   Ordered application-defined source resolver chain. It does not perform
   filesystem I/O by itself.

``PreprocessorController``
   Extensible pre-lexer directive/pragma processing. Per-run mutable state lives
   in ``PreprocessorContext`` rather than in the controller.

``AtomicController``
   Packages common expression features: literals and operators.

``EssentialsController``
   Packages common imperative features: program roots, scopes, variables,
   functions, returns, conditionals, and loops.

``controlflow`` facilities
   Helpers for language-defined statements, generic runtime control signals,
   and loop iteration guards. They deliberately reuse ``EngineFeature`` rather
   than introducing another controller.

``TypeController``
   Independent optional controller for generic language-defined types, aliases,
   conversions, and typing semantics attached to Atomic literal/operation
   features. It owns its own validation/finalization lifecycle and is not stored
   inside ``EngineController``.

``MemoryController``
   Independent optional bit-precise memory asset layered over Types/Layout/Storage.
   It tracks address spaces, regions, allocations, lifetimes, and typed references
   while raw address-space behavior remains pluggable through ``BitAccess``.

Atomic and Essentials configure the Engine; Types and Memory remain optional
assets with their own lifecycle. Source/preprocessing is different: it is part
of the Engine because it changes how a logical source reaches the lexer, but it
remains dormant unless the source APIs are used.

Choose the smallest layer you need
----------------------------------

A calculator may use only Engine + Atomic and call ``parse(text)``. A dynamically
typed scripting language may add Essentials. A statically typed language can
add TypeController explicitly. A project that needs includes or conditional
compilation can install preprocessing and use ``parseSource``. A compiler
experiment may use Engine and custom control-flow/lowering rules without
Essentials at all.

That ability to stop at the level you need is a central design goal of NovaC.
