#ifndef CSVSETTINGS_DIALOG_H
#define CSVSETTINGS_DIALOG_H

#include "settingwindow.hpp"
#include <QStandardItem>

#include "ui_settingstab.h"

namespace CSVSettings {

    class Page;

    class Dialog : public SettingWindow, private Ui::TabWidget
    {
        Q_OBJECT

        //QListWidget *mPageListWidget;
        //ResizeableStackedWidget *mStackedWidget;
        bool mDebugMode;

    public:

        explicit Dialog (QTabWidget *parent = 0);

        ///Enables setting debug mode.  When the dialog opens, a page is created
        ///which displays the SettingModel's contents in a Tree view.
        void enableDebugMode (bool state, QStandardItemModel *model = 0);

    protected:

        /// Settings are written on close
        void closeEvent (QCloseEvent *event);

        void setupDialog();

        bool eventFilter(QObject *target, QEvent *event);

    private:

        void buildPages();

    public slots:

        void show();

    private slots:

        void slotOverrideToggled(bool checked);
        void slotRendererChanged(const QString &renderer);

    signals:

        void toggleStatusBar(bool checked); // FIXME: maybe not needed
    };
}
#endif // CSVSETTINGS_DIALOG_H
