Custom control-flow statement
=============================

This example adds an ``unless`` statement without installing or modifying
Essentials. It uses :cpp:func:`novac::controlflow::statement` to install the
lexer trigger, AST schema, parser rule, runtime handler, and HIR lowerer through
one regular ``EngineFeature``.

The tiny host language only knows ``false`` and a ``hit`` statement. The custom
statement:

.. code-block:: text

   unless false hit

executes ``hit`` once at runtime. Its HIR lowerer also creates a real
three-block control-flow graph: entry, body, and exit.

.. literalinclude:: ../../examples/custom_control_flow.cpp
   :language: cpp
   :linenos:

Expected output
---------------

.. code-block:: text

   runtime hits=1
   hir blocks=3
   entry -> branch.false
   unless.body -> jump
   unless.exit -> open

The operation names ``branch.false`` and ``jump`` belong to this example. NovaC
provides generic blocks and terminators but does not impose a universal HIR
instruction vocabulary.
