#ifndef SERVERMODEL_FONTMODEL_HPP
#define SERVERMODEL_FONTMODEL_HPP

#include <QObject>
#include <vector>
#include <QString>
#include <QAbstractTableModel>

struct ServerData
{
    QString addr;
    int players, maxPlayers;
    int ping;
    QString hostName;
    QString modName;
    enum IDS
    {
        ADDR,
        HOSTNAME,
        PLAYERS,
        MAX_PLAYERS,
        MODNAME,
        PING,
        LAST
    };
};

class ServerModel: public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit ServerModel(QObject *parent = 0);
    ~ServerModel();
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    bool insertRows(int row, int count, const QModelIndex &index = QModelIndex());
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;


public:
    //QHash<int, QByteArray> roles;
    QVector<ServerData> myData;
};


#endif //SERVERMODEL_FONTMODEL_HPP
