
#include "instancemovemode.hpp"

CSVRender::InstanceMoveMode::InstanceMoveMode (QWidget *parent)
: ModeButton (QIcon (QPixmap (":scenetoolbar/transform-move")),
  "Move selected instances"
  "<ul><li>Use {scene-edit-primary} to move instances around freely</li>"
  "<li>Use {scene-edit-secondary} to move instances around within the grid</li>"
  "</ul>"
  "<font color=Red>Grid move not implemented yet</font color>",
  parent)
{}
