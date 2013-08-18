#ifndef DATAFILESPAGE_H
#define DATAFILESPAGE_H

#include <QWidget>
#include <QModelIndex>

#include "ui_datafilespage.h"
#include "components/esxselector/view/contentselector.hpp"

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

class DataFilesPage : public EsxView::ContentSelector
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

    // Action slots
    void on_newProfileAction_triggered();
    void on_deleteProfileAction_triggered();

private slots:

private:

    QMenu *mContextMenu;

    Files::ConfigurationManager &mCfgMgr;

    GameSettings &mGameSettings;
    LauncherSettings &mLauncherSettings;

    TextInputDialog *mNewProfileDialog;

    void setPluginsCheckstates(Qt::CheckState state);

    void createActions();
    void setupDataFiles();
    void setupConfig();
    void readConfig();

    void loadSettings();

};

#endif
