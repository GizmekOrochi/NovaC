Repository structure
====================

.. code-block:: text

   NovaC/
   |-- Makefile
   |-- README.md
   |-- docs/
   |   |-- config/
   |   |   |-- conf.py
   |   |   |-- Doxyfile
   |   |   |-- requirements.txt
   |   |   |-- examples/           # real, compile-tested documentation examples
   |   |   `-- source/             # hand-written Sphinx pages
   |   `-- documentation/          # generated HTML
   |-- src/
   |   |-- NovaC.hpp               # umbrella header
   |   |-- include/novac/          # public API
   |   `-- core/novac/             # implementation
   `-- tests/
       |-- engine/
       `-- assets/
           |-- atomic/
           |-- essentials/
           `-- types/

Public API
----------

``src/include/novac`` is the public API tree. Like Atomic and Essentials, the Types
asset keeps its controller at the asset root and only groups supporting concepts in
small ``model/`` and ``semantics/`` subdirectories. Doxygen scans the tree recursively
so Breathe can expose the documented symbols in the API section.

Implementation
--------------

``src/core/novac`` mirrors the public hierarchy and contains implementation
sources.

Documentation source vs output
------------------------------

``docs/config`` is source-controlled documentation input and build
configuration. ``docs/documentation`` is generated output and can be recreated
with ``make doc``.
