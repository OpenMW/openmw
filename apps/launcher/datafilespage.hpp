#ifndef DATAFILESPAGE_H
#define DATAFILESPAGE_H

#include <QWidget>
#include <QModelIndex>

#include "ui_datafilespage.h"

class QSortFilterProxyModel;
class QAbstractItemModel;
class QAction;
class QMenu;

class DataFilesModel;
class TextInputDialog;
class GameSettings;
class LauncherSettings;
class PluginsProxyModel;

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
    void newProfile();
    void deleteProfile();
    void check();
    void uncheck();
    void refresh();

private slots:
    void slotCurrentIndexChanged(int index);

private:
    DataFilesModel *mDataFilesModel;

    PluginsProxyModel *mPluginsProxyModel;
    QSortFilterProxyModel *mMastersProxyModel;

    QSortFilterProxyModel *mFilterProxyModel;

//    QTableView *mMastersTable;
//    QTableView *mPluginsTable;


//    QToolBar *mProfileToolBar;
    QMenu *mContextMenu;
//    QSplitter *mSplitter;

    QAction *mNewProfileAction;
    QAction *mDeleteProfileAction;
    QAction *mCheckAction;
    QAction *mUncheckAction;

//    QAction *mMoveUpAction;
//    QAction *mMoveDownAction;
//    QAction *mMoveTopAction;
//    QAction *mMoveBottomAction;

    Files::ConfigurationManager &mCfgMgr;

    GameSettings &mGameSettings;
    LauncherSettings &mLauncherSettings;

    TextInputDialog *mNewProfileDialog;

    void setMastersCheckstates(Qt::CheckState state);
    void setPluginsCheckstates(Qt::CheckState state);

    void createActions();
    void setupDataFiles();
    void setupConfig();
    void readConfig();

    void loadSettings();

};

#endif
