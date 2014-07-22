
#include "pushbutton.hpp"

#include <QMouseEvent>
#include <QKeyEvent>

void CSVWidget::PushButton::setExtendedToolTip (const QString& text)
{
    QString tooltip = text;

    if (tooltip.isEmpty())
        tooltip = "(Tool tip not implemented yet)";

    switch (mType)
    {
        case Type_TopMode:

            tooltip +=
                "<p>(left click to change mode)";

            break;

        case Type_Mode:

            tooltip +=
                "<p>(left click to activate,"
                "<br>shift-left click to activate and keep panel open)";

            break;
    }

    setToolTip (tooltip);
}

void CSVWidget::PushButton::keyPressEvent (QKeyEvent *event)
{
    if (event->key()!=Qt::Key_Shift)
        mKeepOpen = false;

    QPushButton::keyPressEvent (event);
}

void CSVWidget::PushButton::keyReleaseEvent (QKeyEvent *event)
{
    if (event->key()==Qt::Key_Return || event->key()==Qt::Key_Enter)
    {
        mKeepOpen = event->modifiers() & Qt::ShiftModifier;
        emit clicked();
    }

    QPushButton::keyReleaseEvent (event);
}

void CSVWidget::PushButton::mouseReleaseEvent (QMouseEvent *event)
{
    mKeepOpen = event->button()==Qt::LeftButton && (event->modifiers() & Qt::ShiftModifier);
    QPushButton::mouseReleaseEvent (event);
}

CSVWidget::PushButton::PushButton (const QIcon& icon, Type type, const QString& tooltip,
    QWidget *parent)
: QPushButton (icon, "", parent), mKeepOpen (false), mType (type), mToolTip (tooltip)
{
    setCheckable (type==Type_Mode);
    setExtendedToolTip (tooltip);
}

CSVWidget::PushButton::PushButton (Type type, const QString& tooltip, QWidget *parent)
: QPushButton (parent), mKeepOpen (false), mType (type), mToolTip (tooltip)
{
    setCheckable (type==Type_Mode);
    setExtendedToolTip (tooltip);
}

bool CSVWidget::PushButton::hasKeepOpen() const
{
    return mKeepOpen;
}

QString CSVWidget::PushButton::getBaseToolTip() const
{
    return mToolTip;
}