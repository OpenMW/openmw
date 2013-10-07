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
namespace ContentSelectorView { class ContentSelector; }

class DataFilesPage : public QWidget
{
    Q_OBJECT

    ContentSelectorView::ContentSelector *mSelector;

public:
    DataFilesPage(Files::ConfigurationManager &cfg, GameSettings &gameSettings, LauncherSettings &launcherSettings, QWidget *parent = 0);

    QAbstractItemModel* profilesModel() const;
    int profilesIndex() const;

    void writeConfig(QString profile = QString());
    void saveSettings(const QString &profile = "");
    void loadSettings();

signals:
    void signalProfileChanged(int index);

public slots:
     //void showContextMenu(const QPoint &point);


private slots:

    void slotAddNewProfile(const QString &profile);
    void slotProfileChangedByUser(const QString &previous, const QString &current);
    void slotProfileChanged(int);
    void slotProfileRenamed(const QString &previous, const QString &current);
    void slotProfileDeleted(const QString &item);

private:

    QMenu *mContextMenu;

    Files::ConfigurationManager &mCfgMgr;

    GameSettings &mGameSettings;
    LauncherSettings &mLauncherSettings;

    void setPluginsCheckstates(Qt::CheckState state);

    void setupDataFiles();
    void setupConfig();
    void readConfig();
    void removeProfile (const QString &profile);
    void changeProfiles(const QString &previous, const QString &current, bool savePrevious = true);
};

#endif
