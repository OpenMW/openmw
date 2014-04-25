#ifndef CSVSETTINGS_RESIZEABLESTACKEDWIDGET_HPP
#define CSVSETTINGS_RESIZEABLESTACKEDWIDGET_HPP

#include <QStackedWidget>

class QListWidgetItem;

namespace CSVSettings
{
    class ResizeableStackedWidget : public QStackedWidget
    {
        Q_OBJECT

    public:
        explicit ResizeableStackedWidget(QWidget *parent = 0);

        void addWidget(QWidget* pWidget);

        void changePage (int, int);
    };
}

#endif // CSVSETTINGS_RESIZEABLESTACKEDWIDGET_HPP
