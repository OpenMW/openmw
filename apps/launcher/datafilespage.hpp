#ifndef DATAFILESPAGE_H
#define DATAFILESPAGE_H

#include <QWidget>
#include <QModelIndex>
#include "utils/profilescombobox.hpp"
#include <components/files/collections.hpp>


class QTableView;
class QSortFilterProxyModel;
class QSettings;
class QAction;
class QToolBar;
class QMenu;
class ProfilesComboBox;
class DataFilesModel;

class TextInputDialog;

namespace Files { struct ConfigurationManager; }

class DataFilesPage : public QWidget
{
    Q_OBJECT

public:
    DataFilesPage(Files::ConfigurationManager& cfg, QWidget *parent = 0);

    ProfilesComboBox *mProfilesComboBox;

    void writeConfig(QString profile = QString());
    bool setupDataFiles();

public slots:
    void setCheckState(QModelIndex index);

    void filterChanged(const QString filter);
    void showContextMenu(const QPoint &point);
    void profileChanged(const QString &previous, const QString &current);
    void profileRenamed(const QString &previous, const QString &current);
    void updateOkButton(const QString &text);

    // Action slots
    void newProfile();
    void deleteProfile();
//    void moveUp();
//    void moveDown();
//    void moveTop();
//    void moveBottom();
    void check();
    void uncheck();
    void refresh();

private:
    DataFilesModel *mMastersModel;
    DataFilesModel *mPluginsModel;

    QSortFilterProxyModel *mPluginsProxyModel;

    QTableView *mMastersTable;
    QTableView *mPluginsTable;

    QToolBar *mProfileToolBar;
    QMenu *mContextMenu;

    QAction *mNewProfileAction;
    QAction *mDeleteProfileAction;

//    QAction *mMoveUpAction;
//    QAction *mMoveDownAction;
//    QAction *mMoveTopAction;
//    QAction *mMoveBottomAction;
    QAction *mCheckAction;
    QAction *mUncheckAction;

    Files::ConfigurationManager &mCfgMgr;
    Files::PathContainer mDataDirs;
    Files::PathContainer mDataLocal;

    QSettings *mLauncherConfig;

    TextInputDialog *mNewProfileDialog;

//    const QStringList checkedPlugins();
//    const QStringList selectedMasters();

    void createActions();
    void setupConfig();
    void readConfig();

};

#endif
