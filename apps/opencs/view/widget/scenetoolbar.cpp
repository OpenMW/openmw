
#include "scenetoolbar.hpp"

#include <QVBoxLayout>

#include "scenetool.hpp"

void CSVWidget::SceneToolbar::focusInEvent (QFocusEvent *event)
{
    QWidget::focusInEvent (event);

    if (mLayout->count())
        dynamic_cast<QWidgetItem&> (*mLayout->itemAt (0)).widget()->setFocus();
}

CSVWidget::SceneToolbar::SceneToolbar (int buttonSize, QWidget *parent)
: QWidget (parent), mButtonSize (buttonSize), mIconSize (buttonSize-6)
{
    setFixedWidth (mButtonSize);

    mLayout = new QVBoxLayout (this);
    mLayout->setAlignment (Qt::AlignTop);

    mLayout->setContentsMargins (QMargins (0, 0, 0, 0));

    setLayout (mLayout);
}

void CSVWidget::SceneToolbar::addTool (SceneTool *tool)
{
    mLayout->addWidget (tool, 0, Qt::AlignTop);
}

int CSVWidget::SceneToolbar::getButtonSize() const
{
    return mButtonSize;
}

int CSVWidget::SceneToolbar::getIconSize() const
{
    return mIconSize;
}