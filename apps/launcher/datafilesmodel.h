#ifndef DATAFILESMODEL_H
#define DATAFILESMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

class DataFilesItem;

class DataFilesModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    //DataFilesModel(const QString &data, QObject *parent = 0);
    DataFilesModel(QObject *parent = 0);
    ~DataFilesModel();

    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

private:
    //void setupModelData(const QStringList &lines, TreeItem *parent);

    DataFilesItem *rootItem;
};

#endif