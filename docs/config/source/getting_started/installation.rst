Installation
============

Requirements
------------

NovaC is currently a source-distributed C++20 project. To build the repository
itself you need:

* a C++20 compiler (the provided Makefile uses ``g++``);
* GNU Make;
* a Unix-like shell for the current Makefile commands.

To build the documentation you additionally need:

* Python 3 with ``venv`` support;
* Doxygen available as ``doxygen`` on ``PATH``;
* an Internet connection on the first ``make doc`` so the private Sphinx
  environment can install its Python dependencies.

Build NovaC
-----------

From the repository root:

.. code-block:: console

   $ make

The current repository Makefile builds ``bin/NovaC``.

Run the tests
-------------

.. code-block:: console

   $ make test

The test target builds and executes ``bin/tests``.

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

Strict documentation build
--------------------------

Before a release, use:

.. code-block:: console

   $ make doc-strict

This asks Sphinx to treat warnings as errors. It is useful for catching broken
references or malformed documentation pages.

Cleaning
--------

``make doc-clean``
   Removes generated HTML and Doxygen XML.

``make doc-clean-all``
   Also removes the private Python virtual environment.

``make clean``
   Removes normal binaries and generated documentation while keeping the
   Sphinx environment for faster future builds.

.. important::

   Building the repository and **using NovaC from another project are two
   different things**. The current V1 Makefile does not install a system
   library or CMake package. See :doc:`integration` for the supported practical
   integration model.
