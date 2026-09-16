Minimal expression language
===========================

This tested example installs only integer literals and addition. It is the
smallest demonstration of NovaC's "install only what you need" philosophy.

.. literalinclude:: ../../examples/minimal.cpp
   :language: cpp
   :linenos:

Expected output:

.. code-block:: text

   42

Notice that Essentials is not installed. There are no variables, functions, or
statements because this language only needs one expression.
