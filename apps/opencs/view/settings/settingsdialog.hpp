#ifndef CSVSETTINGS_SETTINGSDIALOG_H
#define CSVSETTINGS_SETTINGSDIALOG_H

#include <QStandardItem>

#include "ui_settingstab.h"

class QListWidgetItem;

namespace CSMSettings
{
    class Setting;
    class UserSettings;
}

namespace CSVSettings {

    class SettingsDialog : public QTabWidget, private Ui::TabWidget
    {
        Q_OBJECT

        CSMSettings::UserSettings *mModel;

    public:

        SettingsDialog (QTabWidget *parent = 0);

        ///set the model the view uses (instance of UserSettings)
        void setModel (CSMSettings::UserSettings &model)  { mModel = &model; }

    protected:

        ///save settings from the GUI to file
        void saveSettings();

        /// Settings are written on close
        void closeEvent (QCloseEvent *event);

    private:

        ///sets the defined values for the views that have been created
        void setViewValues();

        ///create connections between settings (used for proxy settings)
        void createConnections (const QList <CSMSettings::Setting *> &list);

    public slots:

        void show();

        void rendererChanged();
        void slotStandardToggled(bool checked);
    };
}
#endif // CSVSETTINGS_SETTINGSDIALOG_H
