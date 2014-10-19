#ifndef CSVSETTINGS_DIALOG_H
#define CSVSETTINGS_DIALOG_H

#include "settingwindow.hpp"
#include "resizeablestackedwidget.hpp"
#include <QStandardItem>

class QStackedWidget;
class QListWidget;
class QListWidgetItem;
class QSplitter;

namespace CSVSettings {

    class Page;

    class Dialog : public SettingWindow
    {
        Q_OBJECT

        QListWidget *mPageListWidget;
        ResizeableStackedWidget *mStackedWidget;
        bool mDebugMode;

    public:

        explicit Dialog (QMainWindow *parent = 0);

        ///Enables setting debug mode.  When the dialog opens, a page is created
        ///which displays the SettingModel's contents in a Tree view.
        void enableDebugMode (bool state, QStandardItemModel *model = 0);

    protected:

        /// Settings are written on close
        void closeEvent (QCloseEvent *event);

        void setupDialog();

    private:

        void buildPages();
        void buildPageListWidget (QSplitter *centralWidget);
        void buildStackedWidget (QSplitter *centralWidget);

    public slots:

        void show();

    private slots:

        void slotChangePage (QListWidgetItem *, QListWidgetItem *);
    };
}
#endif // CSVSETTINGS_DIALOG_H
