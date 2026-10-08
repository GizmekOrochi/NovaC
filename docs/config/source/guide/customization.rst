Custom syntax
=============

NovaC's standard assets expose language syntax through option structures. You
can therefore change vocabulary and structural AST names without editing the
Engine.

Essentials option groups
------------------------

:cpp:struct:`novac::assets::essentials::EssentialsControllerOptions` groups the
main statement-level settings.

``core``
   Parse domains, common node kinds, shared field names, braces, parentheses,
   commas, and statement terminators.

``variables``
   Declaration/lookup/assignment node kinds, declaration keyword, assignment
   token, and variable field names.

``controlFlow``
   Conditional/loop keywords, node kinds, field names, and loop iteration
   limit.

``functions``
   Function/return keywords, node kinds, field names, parameter/argument
   fields, and entry-point name.

A complete vocabulary change
----------------------------

.. code-block:: cpp

   novac::assets::essentials::EssentialsControllerOptions options{};
   options.functions.functionKeyword = "fn";
   options.functions.returnKeyword = "give";
   options.functions.mainFunctionName = "start";
   options.variables.letKeyword = "var";

   novac::assets::essentials::EssentialsController essentials{engine, options};
   essentials.installStandardCore();

The corresponding source becomes:

.. code-block:: text

   fn start() {
       var answer = 42;
       give answer;
   }

The tested complete version is in :doc:`../examples/complete_language`.

AST naming is configurable too
------------------------------

Options are not limited to visible keywords. Essentials also exposes node-kind
and field-name configuration. This matters when NovaC is embedded into a larger
tooling ecosystem that already has its own AST naming conventions.

Traits
------

``EssentialsControllerOptions::enforceChildTraits`` enables semantic child
trait restrictions in generated schemas. Traits let independently installed
features express compatibility at a semantic level instead of listing every
possible concrete child node kind.

Customize deliberately
-----------------------

Changing syntax is easy; changing semantics should still be intentional. Keep
your language's options in one configuration function or object so keywords,
node names, and runtime expectations remain consistent across the project.
