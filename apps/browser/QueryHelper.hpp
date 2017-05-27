//
// Created by koncord on 27.05.17.
//


#ifndef OPENMW_QUERYHELPER_HPP
#define OPENMW_QUERYHELPER_HPP


#include <QObject>
#include <QAbstractItemModel>

class QueryHelper : public QObject
{
Q_OBJECT
public:
    explicit QueryHelper(QAbstractItemModel *model);
public slots:
    void refresh();
    void terminate();
private:
    QThread *queryThread;
};

class QueryUpdate : public QObject
{
    friend class QueryHelper;
Q_OBJECT
signals:
    void finished();
public slots:
    void process();
private:
    QAbstractItemModel *_model;
};

#endif //OPENMW_QUERYHELPER_HPP