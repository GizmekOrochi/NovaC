Using NovaC as the compiler framework for TreeLang
==================================================

NovaC can be used as the compiler infrastructure underneath TreeLang without
making TreeLang part of NovaC itself. This distinction is important: NovaC
provides reusable language-building mechanisms (with compilation assets parked), while TreeLang remains responsible for
its own syntax, semantics, intermediate representations, ABI lowering and
runtime behavior.

The intended boundary is:

.. code-block:: text

   NovaC
   +-- source / preprocessing infrastructure
   +-- extensible lexer and parser registries
   +-- generic AST schemas and metadata
   +-- scopes and symbol infrastructure
   +-- type, layout, storage and memory infrastructure
   +-- generic HIR/MIR/IR and lowering infrastructure
   +-- diagnostics
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

The umbrella header exposes the Engine and the active, reusable language
building blocks. The former compilation controller and backend infrastructure
have been moved to a parked asset and are not part of this public header.

Compilation asset is parked
^^^^^^^^^^^^^^^^^^^^^^^^^^^

The old ``CompilationController``, ``CompilationSession``, ``ArtifactStore``,
``PassRegistry`` and backend registry have been extracted into
``novac/assets/compilation/`` (including its ``backend/`` subdirectory).
The asset is not compiled or linked into the
NovaC library and are not exposed by ``NovaC.hpp`` while the compilation layer
is being redesigned. Do not build TreeLang against this parked asset as an
active API.

TreeLang can still compose its own processing pipeline from the Engine's source,
lexer, parser, AST and symbol primitives without a prescribed compiler
controller.

Extensible syntax and AST
^^^^^^^^^^^^^^^^^^^^^^^^^

Lexer rules, parser rules and AST node schemas are registered rather than
hard-coded into the engine. AST nodes also support source spans, metadata and
opaque extension-owned fields. A TreeLang integration can consequently define
its language constructs outside the NovaC core.

Semantic passes, scopes and symbols
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The Engine's ``ScopeGraph`` and ``SymbolTable`` provide lexical relationships
independently from its runtime interpreter. TreeLang can implement its own
pass scheduling and artifact management while the compilation asset is parked.

This means language-specific name resolution, ownership rules, lifetime
analysis or other semantic checks can be implemented as TreeLang passes rather
than as special cases in NovaC.

Two IR choices
^^^^^^^^^^^^^^

Engine provides classic HIR/MIR infrastructure for CFG-oriented execution, analysis
and compiler experiments. A language is not required to force every semantic
concept into that representation, however. Engine generic IR provides modules,
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
      +----> Engine HIR/MIR, for interpretation or analysis
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

   NovaC Engine primitives
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
   parser extension points, metadata, diagnostics, symbol infrastructure,
   type/layout infrastructure and generic Engine IR. Pass scheduling and
   backend interfaces can be revisited in the separate compilation asset.

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
