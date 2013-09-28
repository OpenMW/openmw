
#include "scenetool.hpp"

CSVWorld::SceneTool::SceneTool (QWidget *parent) : QPushButton (parent)
{
    setSizePolicy (QSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed));
    setFixedSize (48, 48);

    connect (this, SIGNAL (clicked()), this, SLOT (openRequest()));
}

void CSVWorld::SceneTool::updateIcon (const QIcon& icon)
{
    setIcon (icon);
}

void CSVWorld::SceneTool::openRequest()
{
    showPanel (parentWidget()->mapToGlobal (pos()));
}
