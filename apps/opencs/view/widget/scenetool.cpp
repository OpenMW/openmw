
#include "scenetool.hpp"

#include "scenetoolbar.hpp"

CSVWidget::SceneTool::SceneTool (SceneToolbar *parent) : QPushButton (parent)
{
    setSizePolicy (QSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed));
    setIconSize (QSize (parent->getIconSize(), parent->getIconSize()));
    setFixedSize (parent->getButtonSize(), parent->getButtonSize());

    connect (this, SIGNAL (clicked()), this, SLOT (openRequest()));
}

void CSVWidget::SceneTool::openRequest()
{
    showPanel (parentWidget()->mapToGlobal (pos()));
}
