Container Widget
================

Wraps around its children. Convenient for creating border-type templates.

Relative size and position don't work for children.

For template children, relative size and position depend on the children's combined size.

Properties
----------

.. list-table::
  :header-rows: 1
  :widths: 20 20 60

  * - name
    - type (default value)
    - description
  * - position
    - util.vector2 (0, 0)
    - | Offsets the position of the widget from its parent's
      | top-left corner in pixels.
  * - size
    - util.vector2 (0, 0)
    - Increases the widget's size in pixels.
  * - relativePosition  
    - util.vector2 (0, 0)
    - | Offsets the position of the widget from its parent's
      | top-left corner as a fraction of the parent's size.
  * - relativeSize
    - util.vector2 (0, 0)
    - Increases the widget's size by a fraction of its parent's size.
  * - anchor
    - util.vector2 (0, 0)
    - | Offsets the widget's position by a fraction of its size.
      | Useful for centering or aligning to a corner.
  * - visible
    - boolean (true)
    - Defines if the widget is visible
  * - propagateEvents
    - boolean (true)
    - Allows base widget events to propagate to the widget's parent.
  * - alpha
    - number (1.0)
    - | Set the opacity of the widget and its contents.
      | If `inheritAlpha` is set to `true`, this becomes the maximum alpha value the widget can take.
  * - inheritAlpha
    - boolean (true)
    - | Modulate `alpha` with parents `alpha`.
      | If the parent has `inheritAlpha` set to `true`, the value after modulating is passed to the child.
