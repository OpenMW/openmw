Flex Widget
===========

Aligns its children along either a column or a row, depending on the `horizontal` property.

Properties
----------

.. list-table::
  :header-rows: 1
  :widths: 20 20 60

  * - name
    - type (default value)
    - description
  * - horizontal
    - bool (false)
    - | Flex aligns its children in a row (main axis is horizontal) if true,
      | otherwise in a column (main axis is vertical).
  * - autoSize
    - bool (true)
    - | If true, Flex will automatically resize to fit its contents.
      | Children can't be relatively position/sized when true.
  * - align
    - ui.ALIGNMENT (Start)
    - Where to align the children in the main axis.
  * - arrange
    - ui.ALIGNMENT (Start)
    - How to arrange the children in the cross axis.

External
--------
.. list-table::
  :header-rows: 1
  :widths: 20 20 60

  * - name
    - type (default value)
    - description
  * - grow
    - float (0)
    - | Grow factor for the child. If there is unused space in the Flex,
      | it will be split between widgets according to this value.
      | Has no effect if `autoSize` is `true`.
  * - stretch
    - float (0)
    - | Stretches the child to a percentage of the Flex's cross axis size.
