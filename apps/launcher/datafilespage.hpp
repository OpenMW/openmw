#ifndef DATAFILESPAGE_H
#define DATAFILESPAGE_H

#include "ui_datafilespage.h"
#include <QWidget>

class QSortFilterProxyModel;
class QAbstractItemModel;
class QMenu;

namespace Files { struct ConfigurationManager; }
namespace ContentSelectorView { class ContentSelector; }

namespace Launcher
{
    class TextInputDialog;
    class GameSettings;
    class LauncherSettings;
    class ProfilesComboBox;

    class DataFilesPage : public QWidget
    {
        Q_OBJECT

        ContentSelectorView::ContentSelector *mSelector;
        Ui::DataFilesPage ui;

    public:
        explicit DataFilesPage (Files::ConfigurationManager &cfg, GameSettings &gameSettings,
                                LauncherSettings &launcherSettings, QWidget *parent = 0);

        QAbstractItemModel* profilesModel() const;

        int profilesIndex() const;

        //void writeConfig(QString profile = QString());
        void saveSettings(const QString &profile = "");
        void loadSettings();

    signals:
        void signalProfileChanged (int index);

    public slots:
        void slotProfileChanged (int index);

    private slots:

        void slotProfileChangedByUser(const QString &previous, const QString &current);
        void slotProfileRenamed(const QString &previous, const QString &current);
        void slotProfileDeleted(const QString &item);

        void on_newProfileAction_triggered();
        void on_deleteProfileAction_triggered();

    private:

        QMenu *mContextMenu;

        Files::ConfigurationManager &mCfgMgr;

        GameSettings &mGameSettings;
        LauncherSettings &mLauncherSettings;

        void setPluginsCheckstates(Qt::CheckState state);

        void buildView();
        void setupDataFiles();
        void setupConfig();
        void readConfig();
        void setProfile (int index, bool savePrevious);
        void setProfile (const QString &previous, const QString &current, bool savePrevious);
        void removeProfile (const QString &profile);
        bool showDeleteMessageBox (const QString &text);
        void addProfile (const QString &profile, bool setAsCurrent);
        void checkForDefaultProfile();
    };
}
#endif
