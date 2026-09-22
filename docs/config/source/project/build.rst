Build system
============

NovaC uses GNU Make for the repository build. The default product is a static
C++20 library that can be linked directly or installed for use from another
project.

Main targets
------------

``make`` / ``make all``
   Build ``lib/libNovaC.a``.

``make tests``
   Build the normal test executable as ``bin/tests``.

``make test``
   Build NovaC, link the full test suite against ``lib/libNovaC.a``, and run it.

``make test-asan``
   Rebuild NovaC and the tests with AddressSanitizer, enable leak detection,
   and stop on the first sanitizer error.

``make test-ubsan``
   Rebuild NovaC and the tests with UndefinedBehaviorSanitizer and stop on the
   first detected undefined behavior.

``make package-config``
   Prepare the CMake package files under ``build/package``.

``make install``
   Install public headers, ``libNovaC.a``, and the NovaC CMake package. The
   installation prefix defaults to ``/usr/local`` and can be changed with
   ``PREFIX``. ``DESTDIR`` is supported for staged/package-manager installs.

``make uninstall``
   Remove files installed by the same ``PREFIX``/``DESTDIR`` layout.

``make doc``
   Generate Doxygen XML and Sphinx HTML into ``docs/documentation``.

``make doc-examples``
   Compile and run the documentation example programs.

``make doc-strict``
   Run the documentation examples, then build the documentation with Sphinx
   warnings treated as errors.

``make release-check``
   Run the normal test suite, ASan, UBSan, and the strict documentation gate.
   This is the minimum local validation expected before a release.

``make doc-clean``
   Remove generated HTML and Doxygen XML.

``make doc-clean-all``
   Also remove the Sphinx virtual environment.

``make clean``
   Remove binaries, libraries, object files, package staging files, and
   generated documentation while keeping the documentation virtual environment.

Parallel and incremental builds
-------------------------------

NovaC compiles the library and tests to individual object files, so normal
builds can use Make parallelism:

.. code-block:: console

   $ make -j
   $ make -j test

Dependency files are generated for the normal library and test objects, so an
incremental rebuild recompiles affected translation units instead of rebuilding
all NovaC sources in one compiler invocation.

Produced library
----------------

The default build creates:

.. code-block:: text

   lib/libNovaC.a

Public headers remain under ``src/include/novac`` in a source checkout. An
installed tree places them under ``<prefix>/include/novac``.

Installation layout
-------------------

For the default prefix, ``make install`` produces the equivalent of:

.. code-block:: text

   /usr/local/include/novac/...
   /usr/local/lib/libNovaC.a
   /usr/local/lib/cmake/NovaC/NovaCConfig.cmake
   /usr/local/lib/cmake/NovaC/NovaCConfigVersion.cmake
   /usr/local/lib/cmake/NovaC/NovaCTargets.cmake

For package construction or local staging, prefer ``DESTDIR`` instead of
installing into the host filesystem:

.. code-block:: console

   $ make install DESTDIR="$PWD/stage" PREFIX=/usr

The staged files will then appear under ``stage/usr``.

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

Installed consumers should normally use the exported CMake package:

.. code-block:: cmake

   find_package(NovaC CONFIG REQUIRED)
   target_link_libraries(my_target PRIVATE NovaC::NovaC)

The imported target also advertises the public include directory and the C++20
compile feature. See :doc:`../getting_started/integration` for source-tree and
installed integration examples.
