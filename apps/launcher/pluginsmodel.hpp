#ifndef PLUGINSMODEL_H
#define PLUGINSMODEL_H

#include <QStandardItemModel>

class PluginsModel : public QStandardItemModel
{
    Q_OBJECT

public:
    PluginsModel(QObject *parent = 0);
    ~PluginsModel() {};

    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

signals:
    void indexesDropped(QVector<QPersistentModelIndex> indexes);

};

#endif