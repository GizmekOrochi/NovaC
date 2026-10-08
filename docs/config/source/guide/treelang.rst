Using NovaC as the compiler framework for TreeLang
==================================================

NovaC can be used as the compiler infrastructure underneath TreeLang without
making TreeLang part of NovaC itself. This distinction is important: NovaC
provides reusable compiler mechanisms, while TreeLang remains responsible for
its own syntax, semantics, intermediate representations, ABI lowering and
runtime behavior.

The intended boundary is:

.. code-block:: text

   NovaC
   +-- source / preprocessing infrastructure
   +-- extensible lexer and parser registries
   +-- generic AST schemas and metadata
   +-- compilation sessions and pass registries
   +-- scopes and symbol infrastructure
   +-- type, layout, storage and memory infrastructure
   +-- classic HIR/MIR when a CFG-oriented representation is appropriate
   +-- generic region/block/operation IR for custom representations
   +-- diagnostics
   +-- backend interfaces
          |
          v
   TreeLang integration
   +-- TreeLang syntax registrations
   +-- TreeLang AST schemas
   +-- TreeLang semantic rules and analyses
   +-- TreeLang-specific IR or lowering stages when required
   +-- ABI lowering and backend implementation
          |
          v
   TreeLang runtime / execution environment

Nothing in NovaC needs to know what a TreeLang construct means. The framework
only needs to expose enough extension points for the TreeLang integration to
install that meaning.

Why the current architecture fits TreeLang
------------------------------------------

Single public entry point
^^^^^^^^^^^^^^^^^^^^^^^^^

A TreeLang compiler can include the complete NovaC public API through one
header:

.. code-block:: cpp

   #include <NovaC.hpp>

The umbrella header exposes both convenience controllers and the lower-level
building blocks. TreeLang therefore does not need a second compiler-specific
NovaC header.

Default compilation is optional
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``CompilationController`` is a convenience recipe for the traditional
source-to-AST-to-HIR-to-MIR path. It is not the definition of a NovaC compiler.
A project can use the defaults, replace individual ``CompilationSteps``, or
skip the controller and compose the lower-level components directly.

That is useful for TreeLang because the language can reuse the standard source,
lexer and parser infrastructure while choosing a different semantic or IR
pipeline later without forking NovaC.

Extensible syntax and AST
^^^^^^^^^^^^^^^^^^^^^^^^^

Lexer rules, parser rules and AST node schemas are registered rather than
hard-coded into the engine. AST nodes also support source spans, metadata and
opaque extension-owned fields. A TreeLang integration can consequently define
its language constructs outside the NovaC core.

Semantic passes, scopes and symbols
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``CompilationSession``, ``ArtifactStore`` and ``PassRegistry`` allow a compiler
to introduce arbitrary analysis and transformation stages. ``ScopeGraph`` and
``SymbolTable`` provide compiler-side lexical relationships independently from
the runtime interpreter.

This means language-specific name resolution, ownership rules, lifetime
analysis or other semantic checks can be implemented as TreeLang passes rather
than as special cases in NovaC.

Two IR choices
^^^^^^^^^^^^^^

NovaC keeps the classic HIR/MIR infrastructure for conventional CFG-oriented
compilers. A language is not required to force every semantic concept into
that representation, however. The generic IR infrastructure provides modules,
regions, blocks, operations, operands, results, successors and metadata that a
language integration can specialize without adding language-specific opcodes
to NovaC itself.

A TreeLang compiler can therefore use a pipeline such as:

.. code-block:: text

   source
      |
      v
   NovaC lexer / parser
      |
      v
   TreeLang AST
      |
      v
   TreeLang semantic passes
      |
      +----> NovaC HIR/MIR, when useful
      |
      +----> custom semantic IR, when useful
      |
      v
   TreeLang backend / ABI lowering

The exact intermediate stages belong to TreeLang, not to NovaC.

Types, layout and memory
^^^^^^^^^^^^^^^^^^^^^^^^

NovaC separates type descriptions, layout calculation, storage and memory
facilities. These components are reusable infrastructure and do not require a
specific stack, heap, garbage collector or language ABI policy. TreeLang can
reuse the parts that fit its compiler while keeping its final execution model
outside NovaC.

Backend and runtime boundary
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A backend consumes compiler artifacts and produces whatever representation the
language needs next. NovaC should not define TreeLang's ABI and should not
require TreeLang to use NovaC's AST runtime interpreter.

For a production TreeLang compiler, a clean separation is:

.. code-block:: text

   NovaC compiler infrastructure
              |
              v
   TreeLang semantics and lowering
              |
              v
   TreeLang ABI / backend
              |
              v
   TreeLang runtime

The existing NovaC runtime remains useful for languages that want AST
interpretation, but it is an optional facility rather than a constraint on
TreeLang.

What belongs where
------------------

The following rule should be used when extending the integration.

**Belongs in NovaC**
   A mechanism is generic enough to be useful to unrelated languages: lexer or
   parser extension points, metadata, diagnostics, pass scheduling, symbol
   infrastructure, generic IR facilities, backend interfaces, type/layout
   infrastructure, and similar compiler machinery.

**Belongs in TreeLang**
   A concept exists because of TreeLang's syntax, semantic model, execution
   rules, ABI or runtime. It should be implemented by the TreeLang integration
   using NovaC extension points rather than added to the NovaC core.

This boundary is deliberate. NovaC is considered suitable for TreeLang because
TreeLang can now be implemented *on top of* the framework, not because NovaC
contains TreeLang-specific concepts.

Recommended development strategy
--------------------------------

Do not continue generalizing NovaC speculatively before implementing TreeLang.
Start with a small real TreeLang frontend and semantic pipeline. When that work
reveals a missing capability, first ask whether the capability is language
independent:

#. If it is generic compiler infrastructure, improve NovaC.
#. If it expresses TreeLang semantics or runtime policy, keep it in TreeLang.
#. Prefer replacing or composing a controller/step over changing NovaC's core
   behavior.
#. Keep the single ``NovaC.hpp`` entry point and preserve the ability to work
   directly with low-level components.

This approach keeps NovaC reusable while allowing TreeLang to use an unusual
compiler and execution model without fighting framework assumptions.
