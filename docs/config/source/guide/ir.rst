HIR and MIR: what they are and when to use them
================================================

.. note::

   **Status:** HIR, MIR and generic IR are available through Engine and
   ``<NovaC.hpp>``. They are generic representations usable by interpreters,
   analysis tools and custom pipelines. Compilation sessions and backends are
   one parked compilation asset and are not linked into NovaC.

An interpreter can evaluate AST nodes directly or choose to execute HIR/MIR.
IR is optional and does not force a compiler or code generation step.

Start with the term "IR"
------------------------

**IR** means *Intermediate Representation*. It is an internal representation of
a program placed between the source-language syntax and a later execution or
code-generation stage.

A source program is convenient for humans. An AST is convenient for preserving
syntax structure. An IR is usually designed to make program analysis and
transformation easier.

For general background, see `Intermediate representation on Wikipedia
<https://en.wikipedia.org/wiki/Intermediate_representation>`_. NovaC Engine's HIR and MIR are extensible representations; the external article explains the
general compiler concept, not NovaC's exact API.

AST vs IR
---------

Consider the expression:

.. code-block:: text

   20 + 22

An AST naturally represents syntax as a tree:

.. code-block:: text

       AddExpression
        /        \
      20          22

An instruction-oriented IR may instead describe operations in sequence:

.. code-block:: text

   %0 = const 20
   %1 = const 22
   %2 = add %0, %1
   return %2

The second form is often easier for data-flow analysis, rewriting, optimization,
or translation to another backend.

Why two levels?
---------------

The **Engine** provides two optional generic stages:

``HIR`` — High-Level Intermediate Representation
   The first normalized representation after the AST. It is a good place to
   preserve language-level meaning while removing parser/AST-specific shape.

``MIR`` — Medium-Level Intermediate Representation
   A later representation where operations may be simplified, normalized, or
   adapted toward backend-oriented processing.

The Engine does **not** prescribe what operations your HIR and MIR must
contain. The operation strings and lowering rules belong to your language.

.. code-block:: text

   source
      |
      v
     AST
      |
      | your AST -> HIR lowerers
      v
     HIR
      |
      | your HIR -> MIR lowerers
      v
     MIR
      |
      v
   your optimizer / VM / bytecode generator / native backend / analyzer

What the Engine IR contains
------------------------------------------

The Engine IR subsystem provides reusable structure:

* ``HIRModule`` and ``MIRModule``;
* basic blocks;
* instructions;
* terminators;
* operands;
* literal operands;
* value identifiers;
* value categories;
* HIR/MIR builders;
* a lowering registry;
* AST-to-HIR and HIR-to-MIR passes;
* value remapping during MIR lowering.

It does **not** provide a predefined instruction set or final backend.

A concrete HIRBuilder example
-----------------------------

The following runnable example manually builds the instruction sequence shown
above using the public NovaC package:

.. literalinclude:: ../../examples/ir_builder.cpp
   :language: cpp
   :linenos:

Its output is:

.. code-block:: text

   entry
     const
     const
     add
     return

This example is intentionally manual. It demonstrates what the representation
contains before introducing lowering registries.

Lowering: translating one representation into another
------------------------------------------------------

**Lowering** means translating from a higher-level representation to a lower or
more normalized one.

AST -> HIR
^^^^^^^^^^

Register an HIR lowerer for each AST node kind you want to translate. An
AST-to-HIR lowerer receives the AST node and an ``HIRBuilder`` and emits HIR
instructions.

The pass type is :cpp:class:`novac::ir::ASTLoweringPass` and the registrations
live in :cpp:class:`novac::ir::LoweringRegistry`.

HIR -> MIR
^^^^^^^^^^

Register a MIR lowerer for HIR operation names. A MIR lowerer receives a
``MIRLoweringInput``, a ``MIRBuilder``, and a **read-only**
``MIRLoweringContext``. Use ``input.operands`` for values already translated
from HIR identifiers to MIR identifiers. ``input.source`` contains the original
HIR instruction only for inspection, type information and provenance; **do not**
pass ``input.source.operands`` directly to a MIR builder.

A lowerer returns a ``MIRLoweringResult``. If the HIR instruction produces a
value, the result must be an existing MIR value, or an explicitly elided value
(``MIRLoweringResult::elided()``). A type-changing lowering must explicitly
return ``MIRLoweringResult::converted(value)``. The engine checks value existence,
type compatibility, alias dominance, CFG stability, and duplicate HIR bindings.
A normal instruction lowerer cannot change blocks or emit terminators. A callback
may emit zero, one or many MIR instructions, and a callback that eliminates a
result must ensure that the result has no remaining consumers.

Example::

    registry.mir("const", [](const novac::ir::MIRLoweringInput &input,
                             novac::ir::MIRBuilder &builder,
                             const novac::ir::MIRLoweringContext &,
                             const novac::ir::LoweringRegistry &)
                             -> novac::ir::MIRLoweringResult {
        return builder.emitValue("mir.const", input.operands, input.source.type);
    });

The pass type is :cpp:class:`novac::ir::HIRLoweringPass`. It pre-registers every
block and processes the reachable control-flow graph in depth-first order, then
unreachable blocks. Block IDs, block names and terminators are preserved. Opaque
module, block, instruction and terminator metadata is copied by default;
source instruction metadata is applied to each MIR instruction emitted for it.
Such metadata may contain language-specific HIR references; extensions storing
those references remain responsible for rewriting their contents.

By default, partial modules with unterminated blocks are supported. Use
``lower(hir, {{true}})`` to require termination in **all** blocks, or
``lower(hir, {{true}, false})`` to disable opaque metadata propagation.

**API change:** lowerer callbacks now use ``MIRLoweringInput`` and return
``MIRLoweringResult`` instead of taking a raw HIR ``Instruction`` and returning
an ``optional<ValueId>``. ``MIRLoweringContext::bind`` rejects duplicate bindings.
Adapt third-party callbacks before updating. Compilation-specific transformations
and CFG rewriting remain out of this Engine pass.

Do standard Atomic/Essentials programs automatically produce HIR/MIR?
---------------------------------------------------------------------

No. The standard examples execute the AST directly through the runtime. IR
lowering is an opt-in path for projects that register the necessary lowering
rules.

That distinction is important:

.. code-block:: text

   Normal interpreted language:
   parse -> AST -> eval

   Language with custom compiler pipeline:
   parse -> AST -> your HIR lowerers -> HIR -> your MIR lowerers -> MIR

When should I use IR?
---------------------

Use HIR/MIR when you need one or more of these:

* optimization passes;
* static program analysis;
* normalization before code generation;
* bytecode generation;
* translation to another IR/backend;
* control-flow/basic-block transformations.

Do not add IR merely because compilers often have one. If direct AST execution
fits your language, the shorter pipeline is simpler and fully supported.
