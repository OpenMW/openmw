
#include "instancemovemode.hpp"

CSVRender::InstanceMoveMode::InstanceMoveMode (QWidget *parent)
: ModeButton (QIcon (QPixmap (":placeholder")),
  "Move selected instances"
  "<ul><li>Use primary edit to move instances around freely</li>"
  "<li>Use secondary edit to move instances around within the grid</li>"
  "</ul>"
  "<font color=Red>Grid move not implemented yet</font color>",
  parent)
{}
