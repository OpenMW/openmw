#include "scenetool.hpp"

#include <QMouseEvent>

#include <apps/opencs/view/widget/pushbutton.hpp>

#include "scenetoolbar.hpp"

CSVWidget::SceneTool::SceneTool(SceneToolbar* parent, Type type)
    : PushButton(type, "", parent)
{
    setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    setIconSize(QSize(parent->getIconSize(), parent->getIconSize()));
    setFixedSize(parent->getButtonSize(), parent->getButtonSize());

    connect(this, &SceneTool::clicked, this, &SceneTool::openRequest);
}

void CSVWidget::SceneTool::activate() {}

void CSVWidget::SceneTool::mouseReleaseEvent(QMouseEvent* event)
{
    if (getType() == Type_TopAction && event->button() == Qt::RightButton)
        showPanel(parentWidget()->mapToGlobal(pos()));
    else
        PushButton::mouseReleaseEvent(event);
}

void CSVWidget::SceneTool::openRequest()
{
    if (getType() == Type_TopAction)
        activate();
    else
        showPanel(parentWidget()->mapToGlobal(pos()));
}
