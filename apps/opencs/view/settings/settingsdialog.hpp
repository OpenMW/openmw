#ifndef CSVSETTINGS_SETTINGSDIALOG_H
#define CSVSETTINGS_SETTINGSDIALOG_H

#include <QStandardItem>

#include "ui_settingstab.h"

class QListWidgetItem;

namespace CSVSettings {

    class SettingsDialog : public QTabWidget, private Ui::TabWidget
    {
        Q_OBJECT

    public:

        SettingsDialog (QTabWidget *parent = 0);

        int windowWidth();
        int windowHeight();

        ///Enables setting debug mode.  When the dialog opens, a page is created
        ///which displays the SettingModel's contents in a Tree view.
        //void enableDebugMode (bool state, QStandardItemModel *model = 0);

    protected:

        /// Settings are written on close
        void closeEvent (QCloseEvent *event);

    private:

        void setViewValues();

    public slots:

        void show();
        void rendererChanged();
        void slotFullScreenChanged(int state);
        void slotStandardToggled(bool checked);
    };
}
#endif // CSVSETTINGS_SETTINGSDIALOG_H
