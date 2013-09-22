#ifndef DATAFILESPAGE_H
#define DATAFILESPAGE_H

#include <QWidget>
#include <QModelIndex>

#include "ui_datafilespage.h"
#include "components/contentselector/view/contentselector.hpp"

class QSortFilterProxyModel;
class QAbstractItemModel;
class QAction;
class QMenu;

class TextInputDialog;
class GameSettings;
class LauncherSettings;


namespace Files { struct ConfigurationManager; }

class DataFilesPage : public QWidget, private Ui::DataFilesPage
{
    Q_OBJECT

public:
    DataFilesPage(Files::ConfigurationManager &cfg, GameSettings &gameSettings, LauncherSettings &launcherSettings, QWidget *parent = 0);

    QAbstractItemModel* profilesComboBoxModel();
    int profilesComboBoxIndex();

    void writeConfig(QString profile = QString());
    void saveSettings();

signals:
    void profileChanged(int index);

public slots:
    void setProfilesComboBoxIndex(int index);

    //void showContextMenu(const QPoint &point);
    void profileChanged(const QString &previous, const QString &current);
    void profileRenamed(const QString &previous, const QString &current);
    void updateOkButton(const QString &text);
    void updateViews();
    // Action slots
    void on_newProfileAction_triggered();
    void on_deleteProfileAction_triggered();

private slots:

private:

    QMenu *mContextMenu;
    //ContentSelectorView::ContentSelector mContentSelector;
    ContentSelectorModel::ContentModel *mContentModel;
    Files::ConfigurationManager &mCfgMgr;

    GameSettings &mGameSettings;
    LauncherSettings &mLauncherSettings;

    TextInputDialog *mNewProfileDialog;
    QSortFilterProxyModel *mGameFileProxyModel;
    QSortFilterProxyModel *mAddonProxyModel;

    void setPluginsCheckstates(Qt::CheckState state);

    void createActions();
    void setupDataFiles();
    void setupConfig();
    void readConfig();

    void loadSettings();

    //////////////////////////////////////
    void buildContentModel();
    void buildGameFileView();
    void buildAddonView();
    void buildProfilesView();

    //void addFiles(const QString &path);

    QStringList checkedItemsPaths();

private slots:
    void slotCurrentProfileIndexChanged(int index);
    void slotCurrentGameFileIndexChanged(int index);
    void slotAddonTableItemClicked(const QModelIndex &index);


};

#endif
