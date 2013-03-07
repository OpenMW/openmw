#ifndef PLUGINSPROXYMODEL_HPP
#define PLUGINSPROXYMODEL_HPP

#include <QSortFilterProxyModel>

class QVariant;

class PluginsProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit PluginsProxyModel(QObject *parent = 0);
    ~PluginsProxyModel();

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};

#endif // PLUGINSPROXYMODEL_HPP
