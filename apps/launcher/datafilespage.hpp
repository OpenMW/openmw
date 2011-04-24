#ifndef DATAFILESPAGE_H
#define DATAFILESPAGE_H

#include <QWidget>
#include <QModelIndex>

class QTableWidget;
class QTableView;
class QComboBox;
class QStandardItemModel;
class QItemSelection;
class QItemSelectionModel;
class QStringListModel;

class DataFilesPage : public QWidget
{
    Q_OBJECT

public:
    DataFilesPage(QWidget *parent = 0);

    QComboBox *mProfileComboBox;
    QStringListModel *mProfileModel;

    const QStringList checkedPlugins();
    void writeConfig();

public slots:
    void masterSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void setCheckstate(QModelIndex index);
    void resizeRows();
    void profileChanged(const QString &profile);

private:
    QTableWidget *mMastersWidget;
    QTableView *mPluginsTable;

    QStandardItemModel *mDataFilesModel;
    QStandardItemModel *mPluginsModel;

    QItemSelectionModel *mPluginsSelectModel;

    void setupDataFiles();
    void addPlugins(const QModelIndex &index);
    void removePlugins(const QModelIndex &index);
    void uncheckPlugins();
};

#endif
