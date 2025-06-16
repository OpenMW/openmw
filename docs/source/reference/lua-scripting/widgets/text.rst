Text Widget
===========

Displays text.

Properties
----------

.. list-table::
  :header-rows: 1
  :widths: 20 20 60

  * - name
    - type (default value)
    - description
  * - autoSize
    - boolean (true)
    - | Adjusts this widget's size to fit the text exactly.
      | Ignores `size` and `relativeSize`.
  * - text
    - string ('')
    - The text to display.
  * - textSize
    - number (10)
    - The size of the text.
  * - textColor
    - util.color (``rgb(0, 0, 0)``)
    - The color of the text.
  * - multiline
    - boolean (false)
    - Whether to render text on multiple lines.
  * - wordWrap
    - boolean (false)
    - Whether to break text into lines to fit the widget's width.
  * - textAlignH
    - ui.ALIGNMENT (Start)
    - Horizontal alignment of the text.
  * - textAlignV
    - ui.ALIGNMENT (Start)
    - Vertical alignment of the text.
  * - textShadow
    - boolean (false)
    - Whether to render a shadow behind the text.
  * - textShadowColor
    - util.color (``rgb(0, 0, 0)``)
    - The color of the text shadow.
