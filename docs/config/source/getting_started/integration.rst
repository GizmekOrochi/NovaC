Using NovaC in your own project
===============================

NovaC V1 can be consumed either directly from a source checkout or as an
installed static library with a CMake package. For application projects, the
installed ``NovaC::NovaC`` target is the preferred integration path.

Install NovaC
-------------

Build and install the library and public headers:

.. code-block:: console

   $ make -j
   $ sudo make install PREFIX=/usr/local

This installs ``libNovaC.a``, the ``novac`` public header tree, and the NovaC
CMake package files.

To test an installation without modifying the host system, use a staged prefix:

.. code-block:: console

   $ make install DESTDIR="$PWD/stage" PREFIX=/usr

CMake integration
-----------------

A consumer ``CMakeLists.txt`` can use the exported package directly:

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.16)
   project(MyLanguage LANGUAGES CXX)

   find_package(NovaC CONFIG REQUIRED)

   add_executable(my_language main.cpp)
   target_link_libraries(my_language PRIVATE NovaC::NovaC)

``NovaC::NovaC`` provides:

* the installed ``libNovaC.a`` archive;
* the installed public include directory;
* the C++20 compile feature requirement.

With the default installation prefix, CMake will normally discover NovaC under
``/usr/local``. For a custom prefix, point CMake at it using a standard package
search mechanism such as ``CMAKE_PREFIX_PATH``:

.. code-block:: console

   $ cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/novac
   $ cmake --build build

Direct source-tree linking
--------------------------

When working from a NovaC checkout, build the static library once:

.. code-block:: console

   $ make -j

A small non-CMake consumer can then compile against the source-tree headers and
link the archive explicitly:

.. code-block:: console

   $ NOVAC=/path/to/NovaC
   $ g++ -std=c++20 \
       -I"$NOVAC/src/include" \
       main.cpp \
       "$NOVAC/lib/libNovaC.a" \
       -o my_language

The supported public include root is ``src/include``. Consumer code should
include public headers from the ``novac/...`` hierarchy, for example:

.. code-block:: cpp

   #include <novac/engine/EngineController.hpp>
   #include <novac/assets/atomic/AtomicController.hpp>
   #include <novac/assets/essentials/EssentialsController.hpp>

Do not compile files from ``src/core`` directly into normal consumers. The
static library is the distribution boundary for NovaC's implementation.

A simple consumer Makefile
--------------------------

For a source checkout at ``../third_party/NovaC``:

.. code-block:: make

   CXX ?= g++
   CXXFLAGS ?= -Wall -Wextra -std=c++20
   NOVAC ?= ../third_party/NovaC

   my_language: main.cpp $(NOVAC)/lib/libNovaC.a
    $(CXX) $(CXXFLAGS) -I$(NOVAC)/src/include $< \
        $(NOVAC)/lib/libNovaC.a -o $@

   $(NOVAC)/lib/libNovaC.a:
    $(MAKE) -C $(NOVAC) -j

   run: my_language
   	./my_language

   clean:
   	rm -f my_language

Static linking model
--------------------

NovaC V1 is distributed as a static library. Each executable that links
``libNovaC.a`` receives the required NovaC implementation objects at link time.
The public API remains defined by the headers under ``novac/``; ``src/core`` is
an implementation directory and is not installed.

Package-manager staging
-----------------------

``DESTDIR`` makes it possible to assemble a package tree without changing the
logical installation prefix:

.. code-block:: console

   $ rm -rf stage
   $ make install DESTDIR="$PWD/stage" PREFIX=/usr
   $ find stage/usr -maxdepth 4 -type f

This pattern is suitable for packaging systems that later copy the staged tree
into its final filesystem location.
