#ifndef PLUGINSVIEW_H
#define PLUGINSVIEW_H

#include <QTableView>

#include "pluginsmodel.hpp"

class QSortFilterProxyModel;

class PluginsView : public QTableView
{
    Q_OBJECT
public:
    PluginsView(QWidget *parent = 0);

    PluginsModel* model() const
        { return qobject_cast<PluginsModel*>(QAbstractItemView::model()); }

    void startDrag(Qt::DropActions supportedActions);
    void setModel(QSortFilterProxyModel *model);

public slots:
    void selectIndexes(QVector<QPersistentModelIndex> aIndexes);

};

Q_DECLARE_METATYPE(QVector<QPersistentModelIndex>)

#endif
