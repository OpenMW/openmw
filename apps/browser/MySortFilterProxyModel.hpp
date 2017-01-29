//
// Created by koncord on 30.01.17.
//

#ifndef OPENMW_MYSORTFILTERPROXYMODEL_HPP
#define OPENMW_MYSORTFILTERPROXYMODEL_HPP


#include <QSortFilterProxyModel>

class MySortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const Q_DECL_FINAL;
public:
    MySortFilterProxyModel(QObject *parent);
    void filterFullServer(bool state);
    void filterEmptyServers(bool state);
    void pingLessThan(int maxPing);
private:
    bool filterEmpty, filterFull;
    int maxPing;
};


#endif //OPENMW_MYSORTFILTERPROXYMODEL_HPP
