
#include "scenetoolbar.hpp"

#include <QVBoxLayout>

#include "scenetool.hpp"

CSVWorld::SceneToolbar::SceneToolbar (QWidget *parent) : QWidget (parent)
{
    setFixedWidth (48);

    mLayout = new QVBoxLayout (this);
    mLayout->setAlignment (Qt::AlignTop);

    mLayout->setContentsMargins (QMargins (0, 0, 0, 0));

    setLayout (mLayout);
}

void CSVWorld::SceneToolbar::addTool (SceneTool *tool)
{
    mLayout->addWidget (tool, 0, Qt::AlignTop);
}