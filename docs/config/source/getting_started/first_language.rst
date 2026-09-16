Build your first language
=========================

This tutorial goes beyond the default syntax and creates a small language with:

* ``fn`` instead of ``func``;
* ``give`` instead of ``return``;
* ``var`` instead of ``let``;
* ``start`` instead of ``main``;
* a native ``print`` function;
* recursion, variables, conditionals, and loops.

The complete C++ file used by this documentation is compiled as part of the
documentation example checks, so the listing below is not pseudocode.

.. literalinclude:: ../../examples/custom_language.cpp
   :language: cpp
   :linenos:

The language source embedded in that program is equivalent to:

.. literalinclude:: ../../examples/custom_language.nova
   :language: text
   :linenos:

Expected output
---------------

.. code-block:: text

   factorial(5) =
   120
   sum =
   10
   returned: 130

Why each part exists
--------------------

Create the Engine
^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   novac::controllers::EngineController engine{};

At this point **there is no language yet**. The Engine provides infrastructure,
not a predefined grammar.

Install expression primitives
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   novac::assets::atomic::AtomicController atomic{engine};
   atomic.installStandardCore();

This gives the language literals and common operators.

Choose your statement syntax
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   novac::assets::essentials::EssentialsControllerOptions options{};
   options.functions.functionKeyword = "fn";
   options.functions.returnKeyword = "give";
   options.functions.mainFunctionName = "start";
   options.variables.letKeyword = "var";

These options are language design decisions. They are not special to the
Engine.

Install Essentials
^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   novac::assets::essentials::EssentialsController essentials{engine, options};
   essentials.installStandardCore();

The standard Essentials pack installs the reusable imperative constructs while
respecting the configured syntax.

Add your standard library explicitly
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

NovaC does not silently add a standard library. If your language has ``print``,
you decide how it behaves:

.. code-block:: cpp

   essentials.functionRegistry().native(
       "print",
       [](const novac::ast::NodeList &arguments,
          novac::runtime::RuntimeContext &context)
          -> novac::runtime::Value {
           for(const auto &argument : arguments)
               std::cout << context.eval(*argument).toString();

           std::cout << '\n';
           return novac::runtime::Value::voidValue();
       }
   );

Native arguments are AST nodes. Calling ``context.eval`` gives eager argument
evaluation; keeping them as nodes also allows advanced native functions to
choose their own evaluation strategy.

Run the language
^^^^^^^^^^^^^^^^

.. code-block:: cpp

   const auto program{engine.parse(source)};
   engine.validate(*program);
   const auto result{engine.eval(*program)};

That is the complete front-to-runtime path for an interpreted NovaC language.
No HIR or MIR is required for this workflow.
