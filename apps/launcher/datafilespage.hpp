#ifndef DATAFILESPAGE_H
#define DATAFILESPAGE_H

#include <QWidget>
#include <QModelIndex>
#include "combobox.hpp"

class QTableWidget;
class QStandardItemModel;
class QItemSelection;
class QItemSelectionModel;
class QSortFilterProxyModel;
class QStringListModel;
class QSettings;
class QAction;
class QToolBar;
class QMenu;
class PluginsModel;
class PluginsView;
class ComboBox;

class DataFilesPage : public QWidget
{
    Q_OBJECT

public:
    DataFilesPage(QWidget *parent = 0);

    ComboBox *mProfilesComboBox;
    QSettings *mLauncherConfig;

    const QStringList checkedPlugins();
    const QStringList selectedMasters();
    void setupConfig();
    void readConfig();
    void writeConfig(QString profile = QString());

    void setupDataFiles(const QStringList &paths, bool strict);

public slots:
    void masterSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void setCheckState(QModelIndex index);

    void filterChanged(const QString filter);
    void showContextMenu(const QPoint &point);
    void profileChanged(const QString &previous, const QString &current);

    // Action slots
    void newProfile();
    void copyProfile();
    void deleteProfile();
    void moveUp();
    void moveDown();
    void moveTop();
    void moveBottom();
    void check();
    void uncheck();
    void refresh();

private:
    QTableWidget *mMastersWidget;
    PluginsView *mPluginsTable;

    QStandardItemModel *mDataFilesModel;
    PluginsModel *mPluginsModel;

    QSortFilterProxyModel *mPluginsProxyModel;
    QItemSelectionModel *mPluginsSelectModel;

    QToolBar *mProfileToolBar;
    QMenu *mContextMenu;

    QAction *mNewProfileAction;
    QAction *mCopyProfileAction;
    QAction *mDeleteProfileAction;

    QAction *mMoveUpAction;
    QAction *mMoveDownAction;
    QAction *mMoveTopAction;
    QAction *mMoveBottomAction;
    QAction *mCheckAction;
    QAction *mUncheckAction;

    void addPlugins(const QModelIndex &index);
    void removePlugins(const QModelIndex &index);
    void uncheckPlugins();
    void createActions();

};

#endif
