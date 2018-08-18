Record Filters
##############

Filters are a key element of the OpenMW CS user interface, they allow rapid and
easy access to records presented in all tables. In order to use this
application effectively you need to familiarise yourself with all the concepts
and instructions explained in this chapter. The filter system is somewhat
unusual at first glance, but once you understand the basics it will be fairly
intuitive and easy to use

Filters are a key element to using the OpenMW CS efficiently by allowing you to
narrow down the table entries very quickly and find what you are looking for.
The filter system might appear unusual at first, you don't just type in a word
and get all instances where it occurs, instead filters are first-class objects
in the CS with their own table. This allows you to define very specific filters
for your project and store them on disc to use in the next session. The CS
allows you fine-grained control, you can choose whether to make a filter
persistent between session, only for one session or use a one-off filter by
typing it directly into the filter field.



Terms used
**********

Filter
   A Filter is generally speaking a tool able to filter the elements of a
   table, that is select some elements while discarding others, according to
   some criteria. These criteria are written using their own syntax.

Criterion
   A criterion describes some condition a record needs to satisfy in order to
   be selected. They are written using a special syntax which is explained
   below. We can logically combine multiple criteria in a filter for finer
   control.

Expression
   Expressions are how we perform filtering. They look like functions in a
   programming language: they have a name and accept a number of arguments.
   The expression evaluates to either ``true`` or ``false`` for every record in
   the table. The arguments are expressions themselves.

Arity
   The arity of an expression tells us how many arguments it takes. Expressions
   taking no arguments are called *nullary*, those taking one argument are
   known as *unary* expressions and those taking two arguments are called
   *binary*.



Interface
*********

Above each table there is a text field which is used to enter a filter: either
one predefined by the OpenMW CS developers or one  made by you. Another
important element is the filter table found under *View* → *Filters*. You
should see the default filters made by the OpenMW team in the table. The table
has the columns *Filter*, *Description* and *Modified*.

ID
   A unique name used to refer to this filter. Note that every ID has a
   scope prefix, we will explain these soon.

Modified
   This is the same as for all the other records, it tells us whether the
   filter is *added* or *removed*. Filters are specific to a project instead of
   a content file, they have no effect on the game itself.

Filter
   The actual contents of the filter are given here using the filter syntax.
   Change the expressions to modify what the filter returns.

Description
   A textual description of what the filter does.



Using predefined filters
************************

To use a filter you have to type its ID into the filter field above a table.

For instance, try to opening the objects table (under the world menu) and type
into the filters field ``project::weapons``. As soon as you complete the text
the table will show only the weapons. The string ``project::weapons`` is the ID
of one of the predefined filters. This means that in order to use the filter
inside the table you type its name inside the filter field.

Filter IDs follow these general conventions:

- IDs of filters for a specific record type contain usually the name of a
  specific group. For instance the ``project::weapons`` filter contains the
  term ``weapons``. Plural form is always used.

- When filtering a specific subgroup the ID is prefixed with the name of the
  more general filter. For instance ``project::weaponssilver`` will filter only
  silver weapons and ``project::weaponsmagical`` will filter only magical
  weapons.

- There are few exceptions from the above rule. For instance there are
  ``project::added``, ``project::removed``, ``project::modified`` and
  ``project::base``. You might except something more like
  ``project::statusadded`` but in this case requiring these extra characters
  would not improve readability.

We strongly recommend you take a look at the filters table right now to see
what you can filter with the defaults. Try using the default filters first
before writing you own.



Writing your own filters
************************

As mentioned before, filters are just another type of record in the OpenMW CS.
To create a new filter you will have to add a new record to the *Filters* table
and set its properties to your liking. Filters are created by combining
existing filters into more complex ones.


Scopes
======

Every default filter has the prefix ``project``. This is a *scope*, a mechanism
that determines the lifetime of the filter. These are the supported scopes:

``project::``
   Indicates that the filter is to be used throughout the project in multiple
   sessions. You can restart the CS and the filter will still be there.

``session::``
   Indicates that the filter is not stored between multiple sessions and once
   you quit the OpenMW CS application the filter will be gone. Until then it
   can be found inside the filters table.

Project-filters are stored in an internal project file, not final content file
meant for the player. Keep in mind when collaborating with other modders that
you need to share the same project file.
 


Writing expressions
===================

The syntax for expressions is as follows:

.. code::

   <name>
   <name>(<arg1>)
   <name>(<arg1>, <arg2>, ..., <argn>)

Where ``<name>`` is the name of the expression, such as ``string`` and the
``<arg>`` are expressions themselves. A nullary expression consists only of its
name. A unary expression contains its argument within a pair of parentheses
following the name. If there is more than one argument they are separated by
commas inside the parentheses.

An example of a binary expression is ``string("Record Type", weapon)``; the
name is ``string``, and it takes two arguments which are strings of string
type. The meaning of arguments depends on the expression itself. In this case
the first argument is the name of a record column and the second field is the
values we want to test it against.

Strings are sequences of characters and are case-insensitive. If a string
contains spaces it must be quoted, otherwise the quotes are optional and
ignored.


Constant Expressions
--------------------

These expressions take no arguments and always return the same result.

``true``
   Always evaluates to ``true``.

``false``
   Always evaluates to ``false``.


Comparison Expressions
----------------------

``string(<column>, <value>)``
   The ``<value>`` is a regular expression pattern. The expressions evaluates
   to ``true`` when the value of a record in ``<column>`` matches the pattern.
   Since the majority of the columns contain string values, ``string`` is among
   the most often used expressions. Examples:

   ``string("Record Type", "Weapon")``
      Will evaluate to ``true`` for all records containing ``Weapon`` in the
      *Record Type* column cell.

   ``string("Portable", "true")``
      Will evaluate to ``true`` [#]_ for all records containing word ``true`` inside
      *Portable* column cell.

.. [#] There is no Boolean (``true`` or ``false``) value in the OpenMW CS. You
       should use a string for those.

       
``value(<value>, (<lower>, <upper>))``
   Match a value type, such as a number, with a range of possible values. The
   argument ``<value>`` is the string name of the value we want to compare, the
   second argument is a pair of lower and upper bounds for the range interval.

   One can use either parentheses ``()`` or brackets ``[]`` to surround the
   pair. Brackets are inclusive and parentheses are exclusive. We can also mix
   both styles:

   .. code::

      value("Weight", [20, 50))

   This will match any objects with a weight greater or equal to 20 and
   strictly less than 50.


Logical Expressions
-------------------

``not <expression>``
   Logically negates the result of an expression. If ``<expression>`` evaluates
   to ``true`` the negation is ``false``, and if ``<expression>`` evaluates to
   ``false`` the negation is ``true``. Note that there are no parentheses
   around the argument.

``or(<expr1>, <expr2>, ..., <exprN>)``
   Logical disjunction, evaluates to ``true`` if at least one argument
   evaluates to ``true`` as well, otherwise the expression evaluates to
   ``false``.

   As an example assume we want to filter for both NPCs and creatures; the
   expression for that use-case is

   .. code::
      
      or(string("record type", "npc"), string("record type", "creature"))

   In this particular case only one argument can evaluate to ``true``, but one
   can write expressions where multiple arguments can be ``true`` at a time.

``or(<expr1>, <expr2>, ..., <exprN>)``
   Logical conjunction, evaluates to ``true`` if and only if all arguments
   evaluate to ``true`` as well, otherwise the expression evaluates to
   ``false``.

   As an example assume we want to filter for weapons weighting less than a hundred
   units The expression for that use-case is

   .. code::
      
      and(string("record type", "weapon"), value("weight", (0, 100)))


Anonymous filters
=================

Creating a whole new filter when you only intend to use it once can be
cumbersome. For that reason the OpenMW CS supports *anonymous* filters which
can be typed directly into the filters field of a table. They are not stored
anywhere, when you clear the field the filter is gone forever.

In order to define an anonymous filter you type an exclamation mark as the
first character into the field followed by the filter definition (e.g.
``!string("Record Type", weapon)`` to filter only for weapons).



Creating and saving filters
***************************

Filters are managed the same way as other records: go to the filters table,
right click and select the option *Add Record* from the context menu. You are
given a choice between project- or session scope. Choose the scope from the
dropdown and type in your desired ID for the filter. A newly created filter
does nothing since it still lacks expressions. In order to add your queries you
have to edit the filter record.


Replacing the default filters set
=================================

OpenMW CS allows you to substitute the default filter set for the entire
application. This will affect the default filters for all content files that
have not been edited on this computer and user account.

Create a new content file, add the desired filters, remove the undesired ones
and save. Now rename the *project* file to ``defaultfilters`` and make sure the
``.omwaddon.project`` file extension is removed. This file will act as a
template for all new files from now on. If you wish to go back to the
old default set rename or remove this custom file.
