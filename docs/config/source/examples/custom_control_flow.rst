Custom control-flow statement
=============================

This example adds an ``unless`` statement without installing or modifying
Essentials. It uses :cpp:func:`novac::controlflow::statement` to install the
lexer trigger, AST schema, parser rule and runtime handler through
one regular ``EngineFeature``.

The tiny host language only knows ``false`` and a ``hit`` statement. The custom
statement:

.. code-block:: text

   unless false hit

executes ``hit`` once at runtime. Compilation and IR lowering are deliberately
not part of this Engine example.

.. literalinclude:: ../../examples/custom_control_flow.cpp
   :language: cpp
   :linenos:

Expected output
---------------

.. code-block:: text

   runtime hits=1
