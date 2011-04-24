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
class QStringListModel;

class DataFilesPage : public QWidget
{
    Q_OBJECT

public:
    DataFilesPage(QWidget *parent = 0);

    ComboBox *mProfileComboBox;
    QStringListModel *mProfileModel;

    const QStringList checkedPlugins();
    void writeConfig();

public slots:
    void masterSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void setCheckstate(QModelIndex index);
    void resizeRows();
    void profileChanged(const QString &current, const QString &previous);

    const QStringList checkedItems();

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
