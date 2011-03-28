#ifndef DATAFILESMODEL_H
#define DATAFILESMODEL_H

#include <QFileSystemModel>
#include <QFileIconProvider>

#include <QDebug>

class DataFilesModel : public QFileSystemModel
{
public:
    DataFilesModel(QObject *parent = 0);
    ~DataFilesModel() {};

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    Qt::ItemFlags flags(const QModelIndex &index) const;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

//    void sort(int column, Qt::SortOrder order);
    //test
//    void setCheckedItems(const QStringList& files);
//    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
//    void unCheckAll();

//    const QSet<QPersistentModelIndex> getCheckedItems();
//    const QList<QPersistentModelIndex> getCheckedItems();
    const QStringList getCheckedItems();

//    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

//    QSet<QPersistentModelIndex> checkedItems;
//    QList<QPersistentModelIndex> checkedItems;
    QStringList checkedItems;
};

#endif
