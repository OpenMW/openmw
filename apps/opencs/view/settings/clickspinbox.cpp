#include "clickspinbox.hpp"

#include <QMouseEvent>
#include <iostream>

void ClickSpinBox::mouseReleaseEvent(QMouseEvent *e)
{
    std::cout << "emit" << std::endl;
    emit mouseReleased();
}

