Manual IR builder example
=========================

Read :doc:`../guide/ir` before this page if IR/HIR/MIR are unfamiliar.

This example does **not** parse a language. It manually creates a small HIR
module to make the structure concrete.

.. literalinclude:: ../../examples/ir_builder.cpp
   :language: cpp
   :linenos:

Expected output:

.. code-block:: text

   entry
     const
     const
     add
     return

The operation names ``const``, ``add``, and ``return`` are choices made by the
example. NovaC does not force a fixed HIR instruction set.
