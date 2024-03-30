Built-in Setting Renderers
==========================

.. include:: version.rst

textLine
--------

Single line text input

**Argument**

Table with the following optional fields:

.. list-table::
  :header-rows: 1
  :widths: 20 20 60

  * - name
    - type (default)
    - description
  * - disabled
    - bool (false)
    - Disables changing the setting from the UI

checkbox
--------

True / false (yes/no) toggle

**Argument**

Table with the following optional fields:

.. list-table::
  :header-rows: 1
  :widths: 20 20 60

  * - name
    - type (default)
    - description
  * - disabled
    - bool (false)
    - Disables changing the setting from the UI
  * - l10n
    - string ('Interface')
    - Localization context with display values for the true/false values
  * - trueLabel
    - string ('Yes')
    - Localization key to display for the true value
  * - falseLabel
    - string ('No')
    - Localization key to display for the false value

number
------

Numeric input

**Argument**

Table with the following optional fields:

.. list-table::
  :header-rows: 1
  :widths: 20 20 60

  * - name
    - type (default)
    - description
  * - disabled
    - bool (false)
    - Disables changing the setting from the UI
  * - integer
    - bool (false)
    - Only allow integer values
  * - min
    - number (nil)
    - If set, restricts setting values to numbers larger than min
  * - max
    - number (nil)
    - If set, restricts setting values to numbers smaller than max

select
------

A small selection box with two next / previous arrows on the sides

**Argument**

Table with the following optional fields:

.. list-table::
  :header-rows: 1
  :widths: 20 20 60

  * - name
    - type (default)
    - description
  * - disabled
    - bool (false)
    - Disables changing the setting from the UI
  * - l10n
    - string (required)
    - Localization context with display values for items
  * - items
    - #list<string> ({})
    - List of options to choose from, all the viable values of the setting

color
-----

Hex-code color input with a preview

**Argument**

Table with the following optional fields:

.. list-table::
  :header-rows: 1
  :widths: 20 20 60

  * - name
    - type (default)
    - description
  * - disabled
    - bool (false)
    - Disables changing the setting from the UI

inputBinding
------------

Allows the user to bind inputs to an action or trigger

**Argument**

Table with the following fields:

.. list-table::
  :header-rows: 1
  :widths: 20 20 60

  * - name
    - type (default)
    - description
  * - key
    - #string
    - Key of the action or trigger to which the input is bound
  * - type
    - 'action', 'trigger'
    - Type of the key
