Using NovaC in your own project
===============================

NovaC V1 is currently consumed from source. The repository Makefile builds the
NovaC project itself, but it does not yet install ``libNovaC`` or export a CMake
package.

This page documents the practical integration model that works today.

Required include paths
----------------------

The umbrella header is ``src/NovaC.hpp`` and the public headers are under
``src/include``. Therefore a consumer compiling directly from a checkout needs
both include roots:

.. code-block:: text

   -I/path/to/NovaC/src
   -I/path/to/NovaC/src/include

Minimal direct build
--------------------

Suppose your project contains ``main.cpp`` and NovaC lives in
``../third_party/NovaC``:

.. code-block:: console

   $ NOVAC=../third_party/NovaC
   $ g++ -std=c++20 -Wall -Wextra \
       -I"$NOVAC/src" \
       -I"$NOVAC/src/include" \
       main.cpp \
       $(find "$NOVAC/src/core" -name '*.cpp') \
       -o my_language

This compiles your program together with NovaC's implementation sources.

A simple consumer Makefile
--------------------------

.. code-block:: make

   CXX ?= g++
   CXXFLAGS = -Wall -Wextra -std=c++20
   NOVAC ?= ../third_party/NovaC

   NOVAC_SRC = $(shell find $(NOVAC)/src/core -name '*.cpp')
   NOVAC_INC = -I$(NOVAC)/src -I$(NOVAC)/src/include

   my_language: main.cpp $(NOVAC_SRC)
   	$(CXX) $(CXXFLAGS) $(NOVAC_INC) $^ -o $@

   run: my_language
   	./my_language

   clean:
   	rm -f my_language

Why two include directories?
----------------------------

``#include "NovaC.hpp"`` resolves from ``src``. That umbrella header then
includes headers such as ``novac/engine/EngineController.hpp``, which resolve
from ``src/include``.

For larger projects
-------------------

Compiling every NovaC ``.cpp`` on every build is simple but not efficient. For
a larger project, compile ``src/core`` into object files or a static library once
and link that result into your application.

.. code-block:: console

   $ mkdir -p build/novac
   $ # compile NovaC source files to .o files, then:
   $ ar rcs build/libnovac.a build/novac/*.o

Then link ``build/libnovac.a`` with your language executable.

.. note::

   A future package-oriented build system could expose a proper installed
   library target. Until that exists in the repository, this page describes
   the actual V1 integration contract instead of pretending an installation
   target exists.
