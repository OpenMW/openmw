Iterable types
==============

.. include:: version.rst

List Iterable
-------------

An iterable with defined size and order.

.. code-block:: Lua

   -- can iterate over the list with pairs
   for i, v in pairs(list) do
   -- ...
   end

.. code-block:: Lua

   -- can iterate over the list with ipairs
   for i, v in ipairs(list) do
   -- ...
   end

.. code-block:: Lua

   -- can get total size with the size # operator
   local length = #list

.. code-block:: Lua

   -- can index the list with numbers
   for i = 1, length do
   list[i]
   end

Map Iterable
------------

An iterable with undefined order.

.. code-block:: Lua

   -- can iterate over the map with pairs
   for k, v in pairs(map) do
   -- ...
   end

.. code-block:: Lua

   -- can index the map by key
   map[key]
