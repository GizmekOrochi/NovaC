Typed mini-language example
===========================

This complete example combines Atomic, Essentials and Types to build a small
statically checked language with ``short``, ``int`` and ``long`` variables,
``+`` and ``-`` expressions, and a ``print`` statement.

The source language looks like this:

.. literalinclude:: ../../examples/typed_minilang.nova
   :language: text

The implementation registers primitive types and widening conversions, binds
integer literals to the type system, declares typed arithmetic overloads, then
runs a small AST type checker before executing the program with NovaC's runtime.

.. note::

   ``long`` is modeled as a 64-bit type by ``TypeController``. The current
   ``runtime::Value`` integer representation is still backed by a C++ ``int``,
   so this example demonstrates 64-bit static type semantics without claiming
   full 64-bit runtime integer storage.

Full implementation
-------------------

.. literalinclude:: ../../examples/typed_minilang.cpp
   :language: cpp
   :linenos:
