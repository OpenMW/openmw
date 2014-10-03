#ifndef CSVSETTINGS_SETTINGWINDOW_HPP
#define CSVSETTINGS_SETTINGWINDOW_HPP

#include <QMainWindow>
#include <QList>

#include "../../model/settings/support.hpp"

namespace CSMSettings {
    class Setting;
    class UserSettings;
}

namespace CSVSettings {

    class Page;
    class View;

    typedef QList <Page *> PageList;

    class SettingWindow : public QMainWindow
    {
        Q_OBJECT

        PageList mPages;
        CSMSettings::UserSettings *mModel;

    public:
        explicit SettingWindow(QWidget *parent = 0);

        ///retrieve a reference to a view based on it's page and setting name
        View *findView (const QString &pageName, const QString &setting);

        ///set the model the view uses (instance of UserSettings)
        void setModel (CSMSettings::UserSettings &model)  { mModel = &model; }

    protected:

        ///construct the pages to be displayed in the dialog
        void createPages();

        ///return the list of constructed pages
        const PageList &pages() const     { return mPages; }

        ///save settings from the GUI to file
        void saveSettings();

        ///sets the defined values for the views that have been created
        void setViewValues();

    private:

        ///create connections between settings (used for proxy settings)
        void createConnections (const QList <CSMSettings::Setting *> &list);
    };
}

#endif // CSVSETTINGS_SETTINGWINDOW_HPP
