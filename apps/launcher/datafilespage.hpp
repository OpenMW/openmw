#ifndef DATAFILESPAGE_H
#define DATAFILESPAGE_H

#include <QWidget>
#include <QModelIndex>

class QSortFilterProxyModel;
class QAbstractItemModel;
class QAction;
class QMenu;

class TextInputDialog;
class GameSettings;
class LauncherSettings;


namespace Files { struct ConfigurationManager; }

class DataFilesPage : public QWidget
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
     //void showContextMenu(const QPoint &point);


private slots:

    void slotProfileAdded();
    void slotProfileChanged(const QString &previous, const QString &current);
    void slotProfileRenamed(const QString &previous, const QString &current);
    void slotProfileDeleted(const QString &item);
    void setProfilesComboBoxIndex(int index);

private:

    QMenu *mContextMenu;

    Files::ConfigurationManager &mCfgMgr;

    GameSettings &mGameSettings;
    LauncherSettings &mLauncherSettings;

    void setPluginsCheckstates(Qt::CheckState state);

    void setupDataFiles();
    void setupConfig();
    void readConfig();

    void loadSettings();
};

#endif
