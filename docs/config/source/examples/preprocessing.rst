Source and preprocessing
========================

This example uses an in-memory ``SourceController`` resolver, standard
preprocessing directives, and a language-defined pragma hook.

It demonstrates several important semantics at once:

* an included ``config`` source defines ``FEATURE`` for the rest of the session;
* ``#if FEATURE`` therefore includes ``shared``;
* two different import specifiers resolve to the same canonical source id, so
  that module is imported only once;
* token source locations still point at the files that produced them;
* ``#pragma dialect strict`` dispatches only ``strict`` to the named pragma
  handler.

.. literalinclude:: ../../examples/preprocessing.cpp
   :language: cpp
   :linenos:

Expected output
---------------

.. code-block:: text

   dialect=strict
   from_shared @ shared.nova:1
   from_module @ module.nova:1
   root @ main.nova:8
