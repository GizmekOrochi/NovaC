Installation
============

Requirements
------------

To build NovaC itself you need:

* a C++20 compiler such as GCC or Clang;
* GNU Make;
* ``ar`` and ``ranlib`` for the static library;
* a Unix-like shell for the current Makefile commands.

To build the documentation you additionally need:

* Python 3 with ``venv`` support;
* Doxygen available as ``doxygen`` on ``PATH``;
* an Internet connection on the first documentation build so the private
  Sphinx environment can install its Python dependencies.

Build NovaC
-----------

From the repository root:

.. code-block:: console

   $ make -j

The default target builds the static library:

.. code-block:: text

   lib/libNovaC.a

There is no default NovaC executable target; NovaC is a framework/library meant
to be embedded in language implementations and tooling.

Install NovaC
-------------

Install the public headers, static library, and CMake package using the default
``/usr/local`` prefix:

.. code-block:: console

   $ sudo make install

Choose another prefix when required:

.. code-block:: console

   $ sudo make install PREFIX=/opt/novac

For local staging or package creation, use ``DESTDIR``:

.. code-block:: console

   $ make install DESTDIR="$PWD/stage" PREFIX=/usr

The install includes:

.. code-block:: text

   <prefix>/include/novac/...
   <prefix>/lib/libNovaC.a
   <prefix>/lib/cmake/NovaC/NovaCConfig.cmake
   <prefix>/lib/cmake/NovaC/NovaCConfigVersion.cmake
   <prefix>/lib/cmake/NovaC/NovaCTargets.cmake

Installed CMake projects can then use ``find_package(NovaC CONFIG REQUIRED)``
and link ``NovaC::NovaC``. See :doc:`integration` for a complete consumer
example.

Run the tests
-------------

Run the normal suite:

.. code-block:: console

   $ make test

Run the memory/lifetime and undefined-behavior sanitizer suites separately:

.. code-block:: console

   $ make test-asan
   $ make test-ubsan

The sanitizer targets rebuild both NovaC and the tests with the corresponding
instrumentation.

Build the documentation
-----------------------

.. code-block:: console

   $ make doc

The first documentation build creates ``docs/config/.venv`` and installs
Sphinx, Breathe, Furo, and ``sphinx-copybutton``. Doxygen extracts XML from the
public headers, then Breathe exposes that API to Sphinx.

The generated site is:

.. code-block:: text

   docs/documentation/index.html

Open that file directly in a browser; no web server is required.

Documentation and release validation
------------------------------------

Compile and execute all documentation examples without building the full site:

.. code-block:: console

   $ make doc-examples

Before a release, use the strict documentation target:

.. code-block:: console

   $ make doc-strict

``doc-strict`` first requires the example programs to compile and run, then
builds Sphinx with warnings treated as errors.

The repository-level release gate is:

.. code-block:: console

   $ make release-check

It requires the normal test suite and ``doc-strict`` to pass.

Cleaning
--------

``make clean``
   Removes normal and sanitizer binaries, the static library, object/package
   build files, and generated documentation while preserving the documentation
   virtual environment.

``make doc-clean``
   Removes generated HTML and Doxygen XML.

``make doc-clean-all``
   Also removes the private Python virtual environment.

``make uninstall``
   Removes NovaC files installed by the matching ``PREFIX``/``DESTDIR`` layout.
