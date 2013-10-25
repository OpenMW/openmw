#ifndef DATAFILESPAGE_H
#define DATAFILESPAGE_H

#include <QWidget>

#include "ui_datafilespage.h"

class QSortFilterProxyModel;
class QAbstractItemModel;
class QMenu;

<<<<<<< Updated upstream
class DataFilesModel;
class TextInputDialog;
class GameSettings;
class LauncherSettings;
class PluginsProxyModel;

=======
>>>>>>> Stashed changes
namespace Files { struct ConfigurationManager; }

<<<<<<< Updated upstream
class DataFilesPage : public QWidget, private Ui::DataFilesPage
=======
namespace Launcher
>>>>>>> Stashed changes
{
    class TextInputDialog;
    class GameSettings;
    class LauncherSettings;
    class ProfilesComboBox;

<<<<<<< Updated upstream
public:
    DataFilesPage(Files::ConfigurationManager &cfg, GameSettings &gameSettings, LauncherSettings &launcherSettings, QWidget *parent = 0);

    QAbstractItemModel* profilesComboBoxModel();
    int profilesComboBoxIndex();

    void writeConfig(QString profile = QString());
    void saveSettings();

signals:
    void profileChanged(int index);

public slots:
    void setCheckState(QModelIndex index);
    void setProfilesComboBoxIndex(int index);

    void filterChanged(const QString filter);
    void showContextMenu(const QPoint &point);
    void profileChanged(const QString &previous, const QString &current);
    void profileRenamed(const QString &previous, const QString &current);
    void updateOkButton(const QString &text);
    void updateSplitter();
    void updateViews();

    // Action slots
    void on_newProfileAction_triggered();
    void on_deleteProfileAction_triggered();
    void on_checkAction_triggered();
    void on_uncheckAction_triggered();

private slots:
    void slotCurrentIndexChanged(int index);

private:
    DataFilesModel *mDataFilesModel;

    PluginsProxyModel *mPluginsProxyModel;
    QSortFilterProxyModel *mMastersProxyModel;

    QSortFilterProxyModel *mFilterProxyModel;
=======
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
>>>>>>> Stashed changes

    private:

        QMenu *mContextMenu;

        Files::ConfigurationManager &mCfgMgr;

<<<<<<< Updated upstream
    TextInputDialog *mNewProfileDialog;

    void setMastersCheckstates(Qt::CheckState state);
    void setPluginsCheckstates(Qt::CheckState state);

    void createActions();
    void setupDataFiles();
    void setupConfig();
    void readConfig();

    void loadSettings();

};
=======
        GameSettings &mGameSettings;
        LauncherSettings &mLauncherSettings;

        void setPluginsCheckstates(Qt::CheckState state);
>>>>>>> Stashed changes

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
