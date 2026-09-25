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
       .alignmentBits(1)
       .unsignedType()
       .representation<CardinalDirectionRepresentation>()
       .capability<LayoutCapability, CardinalDirectionLayout>()
       .capability<StorageCapability, CardinalDirectionStorage>()
       .commit();

   types.bindLiteral(arrows, TypeId{"CardinalDirection"});
   types.finalize();

The primitive uses an explicit one-bit alignment, so its two declared bits pack directly with adjacent values.
so ``LayoutController`` preserves the real two-bit storage instead of rounding
the representation to a byte. ``CardinalDirectionStorage`` then implements the
bit-level load/store contract using exact two-bit operations.

The example allocates one ``BitStorage`` of eight bits and stores ``← ↑ → ↓`` at
offsets 0, 2, 4 and 6. The four language values therefore share one real backing
byte. With the example encoding the byte is ``0xE4`` (decimal 228). Parsing each
arrow still produces the same semantic type with a compact value from 0 to 3.

.. literalinclude:: ../../examples/cardinal_direction.cpp
   :language: cpp
   :linenos:
