Calculator-style language
=========================

A calculator can use Engine + Atomic without Essentials.

.. literalinclude:: ../../examples/calculator.cpp
   :language: cpp
   :linenos:

Expected output:

.. code-block:: text

   1 + 2 * 3 -> 7
   10 >= 5 -> true
   true && !false -> true

Why ``std::string`` here?
-------------------------

The array deliberately stores ``std::string`` objects because ``engine.parse``
accepts a ``const std::string &``. String literals/``const char *`` would also
be implicitly convertible, but using ``std::string`` makes the type expected by
the public API explicit and avoids teaching an unnecessary implicit conversion
in introductory documentation.
