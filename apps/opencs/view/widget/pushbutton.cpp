
#include "pushbutton.hpp"

#include <QMouseEvent>
#include <QKeyEvent>

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

CSVWidget::PushButton::PushButton (const QIcon& icon, bool push, QWidget *parent)
: QPushButton (icon, "", parent), mKeepOpen (false)
{
    setCheckable (!push);
}

CSVWidget::PushButton::PushButton (bool push, QWidget *parent)
: QPushButton (parent), mKeepOpen (false)
{
    setCheckable (!push);
}

bool CSVWidget::PushButton::hasKeepOpen() const
{
    return mKeepOpen;
}