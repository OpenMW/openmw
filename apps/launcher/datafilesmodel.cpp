#include <QtGui>

#include "datafilesitem.h"
#include "datafilesmodel.h"

//DataFilesModel::DataFilesModel(const QString &data, QObject *parent)
DataFilesModel::DataFilesModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    QList<QVariant> rootData;
    rootData << " ";
    //rootItem = new DataFilesItem(rootData);
    rootItem = new DataFilesItem(rootData);
    //setupModelData(data.split(QString("\n")), rootItem);
}

DataFilesModel::~DataFilesModel()
{
    delete rootItem;
}

int DataFilesModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<DataFilesItem*>(parent.internalPointer())->columnCount();
    else
        return rootItem->columnCount();
}

QVariant DataFilesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    DataFilesItem *item = static_cast<DataFilesItem*>(index.internalPointer());

    return item->data(index.column());
}

Qt::ItemFlags DataFilesModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant DataFilesModel::headerData(int section, Qt::Orientation orientation,
                                int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

QModelIndex DataFilesModel::index(int row, int column, const QModelIndex &parent)
            const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    DataFilesItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<DataFilesItem*>(parent.internalPointer());

    DataFilesItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex DataFilesModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    DataFilesItem *childItem = static_cast<DataFilesItem*>(index.internalPointer());
    DataFilesItem *parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int DataFilesModel::rowCount(const QModelIndex &parent) const
{
    DataFilesItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<DataFilesItem*>(parent.internalPointer());

    return parentItem->childCount();
}

/*void DataFilesModel::setupModelData(const QStringList &lines, DataFilesItem *parent)
{
    QList<DataFilesItem*> parents;
    QList<int> indentations;
    parents << parent;
    indentations << 0;

    int number = 0;

    while (number < lines.count()) {
        int position = 0;
        while (position < lines[number].length()) {
            if (lines[number].mid(position, 1) != " ")
                break;
            position++;
        }

        QString lineData = lines[number].mid(position).trimmed();

        if (!lineData.isEmpty()) {
            // Read the column data from the rest of the line.
            QStringList columnStrings = lineData.split("\t", QString::SkipEmptyParts);
            QList<QVariant> columnData;
            for (int column = 0; column < columnStrings.count(); ++column)
                columnData << columnStrings[column];

            if (position > indentations.last()) {
                // The last child of the current parent is now the new parent
                // unless the current parent has no children.

                if (parents.last()->childCount() > 0) {
                    parents << parents.last()->child(parents.last()->childCount()-1);
                    indentations << position;
                }
            } else {
                while (position < indentations.last() && parents.count() > 0) {
                    parents.pop_back();
                    indentations.pop_back();
                }
            }

            // Append a new item to the current parent's list of children.
            parents.last()->appendChild(new DataFilesItem(columnData, parents.last()));
        }

        number++;
    }
}*/