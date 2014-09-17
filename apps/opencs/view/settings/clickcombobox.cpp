#include "clickcombobox.hpp"

#include <QMouseEvent>

void ClickComboBox::mouseReleaseEvent(QMouseEvent *e)
{
    emit mouseReleased();
}

