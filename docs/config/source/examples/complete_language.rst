Complete customized language
============================

This is the end-to-end language built in :doc:`../getting_started/first_language`.
It exercises the public documentation rather than relying on private Engine
implementation details.

Features used
-------------

* standard Atomic literals/operators;
* customized function, return, variable, and entry-point vocabulary;
* Essentials program/scopes/variables/control flow/functions;
* native ``print``;
* recursion;
* C-style ``for`` loop;
* AST validation;
* runtime evaluation.

C++ host
--------

.. literalinclude:: ../../examples/custom_language.cpp
   :language: cpp
   :linenos:

Language source
---------------

.. literalinclude:: ../../examples/custom_language.nova
   :language: text
   :linenos:

Expected output
---------------

.. code-block:: text

   factorial(5) =
   120
   sum =
   10
   returned: 130

This example proves an important architectural point: the Engine was not
modified to understand ``fn``, ``give``, ``var``, recursion, or ``print``.
Those choices are assembled above the Engine.
