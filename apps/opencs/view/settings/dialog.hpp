#ifndef CSVSETTINGS_DIALOG_H
#define CSVSETTINGS_DIALOG_H

#include "settingwindow.hpp"

class QStackedWidget;
class QListWidget;
class QListWidgetItem;

namespace CSVSettings {

    class Page;

    class Dialog : public SettingWindow
    {
        Q_OBJECT

        QListWidget *mPageListWidget;
        QStackedWidget *mStackedWidget;

    public:

        explicit Dialog(QMainWindow *parent = 0);

    protected:

        /// Settings are written on close
        void closeEvent (QCloseEvent *event);

        void setupDialog();
       // void addPage (Page *page);

    private:

        void buildPages();
        void buildPageListWidget (QWidget *centralWidget);
        void buildStackedWidget (QWidget *centralWidget);

    public slots:

        /// Called when a different page is selected in the left-hand list widget
        void slotChangePage (QListWidgetItem*, QListWidgetItem*);

        void show();
    };
}
#endif // CSVSETTINGS_DIALOG_H
