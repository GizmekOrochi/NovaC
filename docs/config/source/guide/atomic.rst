Atomic guide
============

Atomic provides reusable **expression-level** language features. It is the
layer that turns a bare Engine into something that can understand values and
operators.

Standard core
-------------

.. code-block:: cpp

   novac::assets::atomic::AtomicController atomic{engine};
   atomic.installStandardCore();

The standard core installs the standard literal set and the standard numeric,
comparison, and logical operations.

Literals
--------

Standard literal features include:

* integer;
* floating-point;
* string;
* boolean.

Each literal feature registers the lexical/parsing/AST/runtime pieces required
for that literal type.

Operators
---------

Standard operator families include:

Numeric
   ``+``, ``-``, ``*``, ``/``, ``%``, and unary negation.

Comparison
   ``==``, ``!=``, ``<``, ``<=``, ``>``, ``>=``.

Logical
   logical AND, OR, and NOT.

Operator handling includes precedence and associativity through the parser
registry, plus runtime operation handlers.

Install only what you need
--------------------------

A tiny integer-addition language can be built without the rest of the standard
core:

.. code-block:: cpp

   novac::assets::atomic::AtomicController atomic{engine};
   atomic.integer();
   atomic.add();

This is the version used by :doc:`../examples/minimal`.

Custom tokens
-------------

Individual operations and packs expose token configuration, so your language
does not need to copy C/C++ spelling. Atomic's role is reusable semantics, not
forcing one visual syntax.

Atomic and Essentials
---------------------

Atomic handles expressions. Essentials handles larger imperative constructs.
For example, Essentials can provide a variable assignment statement while the
expression assigned to that variable is parsed and evaluated by Atomic.


Typed Atomic identifiers
------------------------

Atomic owns the concrete names used by its AST and parser integration. This
keeps the engine independent from asset-level concepts while avoiding repeated
magic strings in Atomic implementations.

Use the typed identifiers when reading or writing Atomic nodes:

.. code-block:: cpp

   #include <NovaC.hpp>

   using namespace novac::assets::atomic;

   node->set(fields::Operation, operationId);
   node->set(fields::Left, left);
   node->set(fields::Right, right);

   const auto leftValue = context.eval(*node->child(fields::Left));

The identifiers are deliberately separated by meaning. For example, the
expression trait, unary-expression field, and expression parser domain may all
use the text ``"expr"`` internally, but they remain different concepts:

.. code-block:: cpp

   novac::assets::traits::Expression
   novac::assets::atomic::fields::Expression
   novac::assets::atomic::domains::Expression

Do not replace these with one global string constant. Keeping them distinct
preserves the boundary between semantic traits, AST structure, and parser
configuration.
