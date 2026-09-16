Build system
============

The repository currently uses a compact GNU Make build.

Main targets
------------

``make`` / ``make all``
   Build ``bin/NovaC``.

``make run``
   Build and run ``bin/NovaC``.

``make tests``
   Build ``bin/tests``.

``make test``
   Build and run the full test suite.

``make doc``
   Generate Doxygen XML and Sphinx HTML into ``docs/documentation``.

``make doc-strict``
   Generate the documentation with Sphinx warnings treated as errors.

``make doc-clean``
   Remove generated HTML and Doxygen XML.

``make doc-clean-all``
   Also remove the Sphinx virtual environment.

``make clean``
   Remove binaries and generated documentation while keeping the documentation
   virtual environment.

Documentation pipeline
----------------------

.. code-block:: text

   public C++ headers
       |
       v
     Doxygen -------> XML
                       |
   hand-written RST    |
          \            /
           \          /
            v        v
             Breathe
                |
                v
              Sphinx
                |
                v
   docs/documentation/index.html

The split is intentional: Doxygen comments are best for member-level API facts;
Sphinx pages are best for concepts, tutorials, architecture, and workflows.

Consumer builds
---------------

The repository build does not currently publish a reusable installed library
target. For using NovaC in another project, see
:doc:`../getting_started/integration`.
