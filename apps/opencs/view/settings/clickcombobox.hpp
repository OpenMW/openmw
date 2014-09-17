#ifndef CSVSETTINGS_CLICKCOMBOBOX_H
#define CSVSETTINGS_CLICKCOMBOBOX_H

#include <QComboBox>
#include <QMouseEvent>

class ClickComboBox : public QComboBox
{
    Q_OBJECT

    public:
        explicit ClickComboBox(QWidget *parent = 0) { }
        void mouseReleaseEvent(QMouseEvent *e);

    signals:
        void mouseReleased();

};

#endif /* CSVSETTINGS_CLICKCOMBOBOX_H */
