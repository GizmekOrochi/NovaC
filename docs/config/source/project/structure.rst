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
           `-- essentials/

Public API
----------

``src/include/novac`` is the public API tree. Doxygen scans it recursively so
Breathe can expose the documented symbols in the API section.

Implementation
--------------

``src/core/novac`` mirrors the public hierarchy and contains implementation
sources.

Documentation source vs output
------------------------------

``docs/config`` is source-controlled documentation input and build
configuration. ``docs/documentation`` is generated output and can be recreated
with ``make doc``.
