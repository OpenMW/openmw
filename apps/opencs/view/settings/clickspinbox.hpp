#ifndef CSVSETTINGS_CLICKSPINBOX_H
#define CSVSETTINGS_CLICKSPINBOX_H

#include <QSpinBox>
#include <QMouseEvent>

class ClickSpinBox : public QSpinBox
{
    Q_OBJECT

    public:
        explicit ClickSpinBox(QWidget *parent = 0) { }
        void mouseReleaseEvent(QMouseEvent *e);

    signals:
        void mouseReleased();

};

#endif /* CSVSETTINGS_CLICKSPINBOX_H */
