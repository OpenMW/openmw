#ifndef CSVSETTINGS_DIALOG_H
#define CSVSETTINGS_DIALOG_H

#include "settingwindow.hpp"
#include <QStandardItem>

class QStackedWidget;
class QListWidget;
class QListWidgetItem;

namespace CSVSettings {

    class Page;

    class StringListItem : public QStandardItem
    {
        QStringList mList;

    public:
        explicit StringListItem(QStringList list)
            : mList (list), QStandardItem()
        {}

        int type() const    { return QStandardItem::UserType; }

        QVariant data(int role = Qt::DisplayRole) const
        { return mList; }

        void setData(const QVariant &value, int role = Qt::DisplayRole)
        { mList = value.toStringList(); }
    };

    class Dialog : public SettingWindow
    {
        Q_OBJECT

        QListWidget *mPageListWidget;
        QStackedWidget *mStackedWidget;
        QStandardItemModel *mModel;
        bool mDebugMode;

    public:

        explicit Dialog(QMainWindow *parent = 0);

        ///Enables setting debug mode.  When the dialog opens, a page is created
        ///which displays the SettingModel's contents in a Tree view.
        void enableDebugMode (bool state, QStandardItemModel *model = 0);

    protected:

        /// Settings are written on close
        void closeEvent (QCloseEvent *event);

        void setupDialog();

    private:

        void buildPages();
        void buildPageListWidget (QWidget *centralWidget);
        void buildStackedWidget (QWidget *centralWidget);
        void addDebugPage();

    public slots:

        /// Called when a different page is selected in the left-hand list widget
        void slotChangePage (QListWidgetItem*, QListWidgetItem*);

        void show();
    };
}
#endif // CSVSETTINGS_DIALOG_H
