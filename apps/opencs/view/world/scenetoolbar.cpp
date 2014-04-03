
#include "scenetoolbar.hpp"

#include <QVBoxLayout>

#include "scenetool.hpp"

CSVWorld::SceneToolbar::SceneToolbar (int buttonSize, QWidget *parent)
: QWidget (parent), mButtonSize (buttonSize), mIconSize (buttonSize-6)
{
    setFixedWidth (mButtonSize);

    mLayout = new QVBoxLayout (this);
    mLayout->setAlignment (Qt::AlignTop);

    mLayout->setContentsMargins (QMargins (0, 0, 0, 0));

    setLayout (mLayout);
}

void CSVWorld::SceneToolbar::addTool (SceneTool *tool)
{
    mLayout->addWidget (tool, 0, Qt::AlignTop);
}

int CSVWorld::SceneToolbar::getButtonSize() const
{
    return mButtonSize;
}

int CSVWorld::SceneToolbar::getIconSize() const
{
    return mIconSize;
}