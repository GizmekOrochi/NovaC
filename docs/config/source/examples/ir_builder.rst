Manual IR builder example
=========================

Read :doc:`../guide/ir` before this page if IR/HIR/MIR are unfamiliar.

Runnable example: it demonstrates a HIR builder available in Engine and
through the public ``<NovaC.hpp>`` header.

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
