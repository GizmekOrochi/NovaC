Cardinal direction literals
===========================

This example defines a language-specific ``CardinalDirection`` type with four
custom arrow literals. The type has a semantic/storage width of two bits, so the
four possible values map naturally to ``00``, ``01``, ``10`` and ``11``.

.. code-block:: text

   ← = 00
   ↑ = 01
   → = 10
   ↓ = 11

The arrows are implemented by a custom ``LiteralFeature`` rather than by
special-casing them in ``TypeController``. The feature installs the four lexer
symbols and produces one AST literal node kind. Types then binds that literal
feature to the custom type:

.. code-block:: cpp

   CardinalDirectionLiteral arrows;
   atomic.use(arrows);

   TypeController types;
   types.definePrimitive("CardinalDirection")
       .bits(2)
       .storageBits(2)
       .alignment(1)
       .unsignedType()
       .representation<CardinalDirectionRepresentation>()
       .commit();

   types.bindLiteral(arrows, TypeId{"CardinalDirection"});
   types.finalize();

Parsing any arrow therefore produces the same semantic type while preserving a
compact direction value from 0 to 3.

.. literalinclude:: ../../examples/cardinal_direction.cpp
   :language: cpp
   :linenos:
