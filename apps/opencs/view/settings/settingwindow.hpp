#ifndef CSVSETTINGS_SETTINGWINDOW_HPP
#define CSVSETTINGS_SETTINGWINDOW_HPP

#include <QMainWindow>
#include <QList>

#include "../../model/settings/support.hpp"

namespace CSMSettings {
    class Setting;
    class SettingManager;
}

namespace CSVSettings {

    class Page;
    class View;

    typedef QList <Page *> PageList;

    class SettingWindow : public QMainWindow
    {
        Q_OBJECT

        PageList mPages;
        CSMSettings::SettingManager *mModel;

    public:
        explicit SettingWindow(QWidget *parent = 0);

        View *findView (const QString &pageName, const QString &setting);
        void setModel (CSMSettings::SettingManager &model)  { mModel = &model; }

    protected:

        virtual void closeEvent (QCloseEvent *event);

        void createPages();

        const PageList &pages() const     { return mPages; }

        void saveSettings();

    private:
        void createConnections (const QList <CSMSettings::Setting *> &list);
    };
}

#endif // CSVSETTINGS_SETTINGWINDOW_HPP
