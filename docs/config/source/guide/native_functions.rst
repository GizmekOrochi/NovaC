Native functions
================

A native function connects language-level code to C++ behavior. Essentials
registers them through
:cpp:class:`novac::assets::essentials::functions::FunctionRegistry`.

A ``print`` example
-------------------

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

Why arguments are AST nodes
---------------------------

The callback receives ``NodeList`` rather than a vector of already evaluated
``Value`` objects. That design leaves evaluation strategy under your control.

For a normal eager function:

.. code-block:: cpp

   const novac::runtime::Value value{context.eval(*arguments[0])};

For a special form, lazy construct, assertion, conditional helper, or custom
macro-like behavior, a native function may inspect or selectively evaluate its
argument nodes instead.

Return values
-------------

Native functions return :cpp:class:`novac::runtime::Value`. A procedure that
produces no useful result returns:

.. code-block:: cpp

   return novac::runtime::Value::voidValue();

Errors and argument checking
----------------------------

The registry does not automatically define the contract of your native
function. If a native requires exactly two arguments or specific runtime types,
validate that inside the callback and report/throw according to your language's
policy.

Entry point
-----------

Essentials also stores the configured program entry point in the function
registry. The default is ``main``; this can be changed to names such as
``start`` through ``FunctionSyntaxOptions::mainFunctionName``.
