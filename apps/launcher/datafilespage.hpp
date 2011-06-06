#ifndef DATAFILESPAGE_H
#define DATAFILESPAGE_H

#include <QWidget>
#include <QModelIndex>
#include "combobox.hpp"

class QTableWidget;
class QTableView;
class ComboBox;
class QStandardItemModel;
class QItemSelection;
class QItemSelectionModel;
class QSortFilterProxyModel;
class QStringListModel;
class QSettings;
class QPushButton;

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
    void setCheckstate(QModelIndex index);
    void filterChanged(const QString filter);
    void profileChanged(const QString &previous, const QString &current);
    void newProfile();
    void copyProfile();
    void deleteProfile();

private:
    QTableWidget *mMastersWidget;
    QTableView *mPluginsTable;

    QStandardItemModel *mDataFilesModel;
    QStandardItemModel *mPluginsModel;

    QSortFilterProxyModel *mPluginsProxyModel;
    QItemSelectionModel *mPluginsSelectModel;

    QPushButton *mNewProfileButton;
    QPushButton *mCopyProfileButton;
    QPushButton *mDeleteProfileButton;

    void addPlugins(const QModelIndex &index);
    void removePlugins(const QModelIndex &index);
    void uncheckPlugins();
};

#endif
