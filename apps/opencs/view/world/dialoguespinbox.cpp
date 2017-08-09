#include "dialoguespinbox.hpp"

#include <QWheelEvent>

CSVWorld::DialogueSpinBox::DialogueSpinBox(QWidget *parent) : QSpinBox(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

void CSVWorld::DialogueSpinBox::focusInEvent(QFocusEvent *event)
{
    setFocusPolicy(Qt::WheelFocus);
    QSpinBox::focusInEvent(event);
}

void CSVWorld::DialogueSpinBox::focusOutEvent(QFocusEvent *event)
{
    setFocusPolicy(Qt::StrongFocus);
    QSpinBox::focusOutEvent(event);
}

void CSVWorld::DialogueSpinBox::wheelEvent(QWheelEvent *event)
{
    if (!hasFocus())
        event->ignore();
    else
        QSpinBox::wheelEvent(event);
}

CSVWorld::DialogueDoubleSpinBox::DialogueDoubleSpinBox(QWidget *parent) : QDoubleSpinBox(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

void CSVWorld::DialogueDoubleSpinBox::focusInEvent(QFocusEvent *event)
{
    setFocusPolicy(Qt::WheelFocus);
    QDoubleSpinBox::focusInEvent(event);
}

void CSVWorld::DialogueDoubleSpinBox::focusOutEvent(QFocusEvent *event)
{
    setFocusPolicy(Qt::StrongFocus);
    QDoubleSpinBox::focusOutEvent(event);
}

void CSVWorld::DialogueDoubleSpinBox::wheelEvent(QWheelEvent *event)
{
    if (!hasFocus())
        event->ignore();
    else
        QDoubleSpinBox::wheelEvent(event);
}
