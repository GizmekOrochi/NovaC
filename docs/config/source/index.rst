NovaC
=====

**NovaC is a modular C++20 framework for building interpreters, DSLs, and
programming languages by composition.**

NovaC does not give you one fixed grammar. It gives you infrastructure and
reusable language features that you assemble into the language you want.

A useful mental model is:

.. code-block:: text

   Your language
       |
       +-- syntax choices
       +-- semantic choices
       +-- native functions
       |
       v
   +-----------------------+
   |       Essentials      |  variables, functions, scopes, control flow
   +-----------+-----------+
               |
   +-----------v-----------+
   |         Atomic        |  literals and operators
   +-----------+-----------+
               |
   +-----------v-----------+
   |         Engine        |  lexer, parser, AST, runtime, optional IR
   +-----------------------+

If you only need expressions, you can stop at Engine + Atomic. If you are
building a complete imperative language, add Essentials. If you want to build
custom compiler passes, the Engine also exposes optional HIR and MIR
infrastructure.

Where should I start?
---------------------

**I want a language running quickly**
   Read :doc:`getting_started/quickstart`, then
   :doc:`getting_started/first_language`.

**I want to use NovaC from my own project**
   Read :doc:`getting_started/integration`.

**I want to understand how NovaC works**
   Read :doc:`guide/architecture`, then :doc:`guide/engine`.

**I do not understand HIR/MIR**
   Read :doc:`guide/ir`. It starts from the definition of an intermediate
   representation before discussing NovaC's classes.

**I want exact classes and signatures**
   Use :doc:`api/index`. The API reference is generated from Doxygen comments
   in the public C++ headers.

.. note::

   The generated HTML lives in ``docs/documentation``. Run ``make doc`` and
   open ``docs/documentation/index.html``.

.. toctree::
   :maxdepth: 2
   :caption: Getting started

   getting_started/installation
   getting_started/quickstart
   getting_started/first_language
   getting_started/integration

.. toctree::
   :maxdepth: 2
   :caption: Concepts and guides

   guide/architecture
   guide/engine
   guide/atomic
   guide/essentials
   guide/types
   guide/native_functions
   guide/customization
   guide/ir
   guide/ownership
   guide/testing
   guide/glossary

.. toctree::
   :maxdepth: 2
   :caption: Tested examples

   examples/minimal
   examples/calculator
   examples/complete_language
   examples/ir_builder
   examples/type_system

.. toctree::
   :maxdepth: 2
   :caption: Project

   project/structure
   project/build
   project/contributing

.. toctree::
   :maxdepth: 2
   :caption: API reference

   api/index
