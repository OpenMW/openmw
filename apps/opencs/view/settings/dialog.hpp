#ifndef CSVSETTINGS_DIALOG_H
#define CSVSETTINGS_DIALOG_H

#include <QMainWindow>

#include "../../model/settings/usersettings.hpp"

class QStackedWidget;
class QListWidget;
class QListWidgetItem;
class QDataWidgetMapper;
class QGroupBox;

namespace CSMSettings { class BinaryWidgetAdapter; }
namespace CSVSettings {

    class Page;

    class Dialog : public QMainWindow
    {
        Q_OBJECT

        QListWidget *mPageListWidget;
        QStackedWidget *mStackedWidget;

    public:

        explicit Dialog(QMainWindow *parent = 0);

    protected:

        /// Settings are written on close
        void closeEvent (QCloseEvent *event);

        void setupStack();
        Page *createPage (const QString &pageName);
        void addPage (Page *page);

    public slots:

        /// Called when a different page is selected in the left-hand list widget
        void slotChangePage (QListWidgetItem*, QListWidgetItem*);

        void show();
    };
}
#endif // CSVSETTINGS_DIALOG_H
