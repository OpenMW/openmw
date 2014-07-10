
#include "pushbutton.hpp"

#include <QMouseEvent>
#include <QKeyEvent>

void CSVWidget::PushButton::keyPressEvent (QKeyEvent *event)
{
    if (event->key()!=Qt::Key_Shift)
        mKeepOpen = false;

    QPushButton::keyPressEvent (event);
}

void CSVWidget::PushButton::mouseReleaseEvent (QMouseEvent *event)
{
    mKeepOpen = event->button()==Qt::LeftButton && (event->modifiers() & Qt::ShiftModifier);
    QPushButton::mouseReleaseEvent (event);
}

CSVWidget::PushButton::PushButton (const QIcon& icon, const QString& text, QWidget *parent)
: QPushButton (icon, text, parent), mKeepOpen (false)
{
}

bool CSVWidget::PushButton::hasKeepOpen() const
{
    return mKeepOpen;
}