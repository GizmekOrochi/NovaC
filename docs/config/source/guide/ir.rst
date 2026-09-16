HIR and MIR: what they are and when to use them
================================================

If you are building an interpreter with ``engine.eval()``, you **do not need
HIR or MIR**. They are optional infrastructure for languages that need compiler
passes, normalization, analysis, optimization, or a custom backend.

Start with the term "IR"
------------------------

**IR** means *Intermediate Representation*. It is an internal representation of
a program placed between the source-language syntax and a later execution or
code-generation stage.

A source program is convenient for humans. An AST is convenient for preserving
syntax structure. An IR is usually designed to make program analysis and
transformation easier.

For general background, see `Intermediate representation on Wikipedia
<https://en.wikipedia.org/wiki/Intermediate_representation>`_. NovaC's HIR and
MIR are its own extensible representations; the external article explains the
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

NovaC provides two optional stages:

``HIR`` — High-Level Intermediate Representation
   The first normalized representation after the AST. It is a good place to
   preserve language-level meaning while removing parser/AST-specific shape.

``MIR`` — Medium-Level Intermediate Representation
   A later representation where operations may be simplified, normalized, or
   adapted toward backend-oriented processing.

NovaC deliberately does **not** prescribe what operations your HIR and MIR must
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

What NovaC provides
-------------------

The IR subsystem provides reusable structure:

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

The following tested example manually builds the instruction sequence shown
above:

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

Register a MIR lowerer for HIR operation names. A MIR lowerer receives one HIR
instruction, a ``MIRBuilder``, and a ``MIRLoweringContext``. The context tracks
how HIR result IDs map to new MIR result IDs.

The pass type is :cpp:class:`novac::ir::HIRLoweringPass`.

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
