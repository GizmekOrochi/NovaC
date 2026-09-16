API reference
=============

These pages are generated from Doxygen comments in NovaC's public C++ headers.
Use the conceptual guides first when learning the framework; use this section
when you need exact classes, methods, options, or signatures.

The API reference is grouped by the three main layers to avoid registering the
same C++ declaration multiple times in Sphinx.

.. toctree::
   :maxdepth: 2

   engine
   atomic
   essentials

.. note::

   If an API entry feels too terse, the right long-term fix is to improve the
   Doxygen comment in the corresponding public header. The generated reference
   will then improve automatically.
