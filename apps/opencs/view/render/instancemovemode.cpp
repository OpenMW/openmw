#include "instancemovemode.hpp"

#include <QIcon>
#include <QPixmap>

#include <apps/opencs/view/widget/modebutton.hpp>

#include <components/misc/scalableicon.hpp>

class QWidget;

CSVRender::InstanceMoveMode::InstanceMoveMode(QWidget* parent)
    : ModeButton(Misc::ScalableIcon::load(":scenetoolbar/transform-move"),
        "Move selected instances"
        "<ul><li>Use {scene-edit-primary} to move instances around freely</li>"
        "<li>Use {scene-edit-secondary} to move instances around within the grid</li>"
        "</ul>",
        parent)
{
}
