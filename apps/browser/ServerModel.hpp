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
    bool needPassw;
    QString version;
    enum IDS
    {
        ADDR,
        HOSTNAME,
        PLAYERS,
        MAX_PLAYERS,
        PASSW,
        MODNAME,
        PING,
        VERSION,
        LAST
    };
};

class ServerModel: public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit ServerModel(QObject *parent = 0);
    ~ServerModel();
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_FINAL;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_FINAL;
    int columnCount(const QModelIndex &parent) const Q_DECL_FINAL;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) Q_DECL_FINAL;

    bool insertRows(int row, int count, const QModelIndex &index = QModelIndex()) Q_DECL_FINAL;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) Q_DECL_FINAL;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_FINAL;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const Q_DECL_FINAL;


public:
    //QHash<int, QByteArray> roles;
    QVector<ServerData> myData;
};


#endif //SERVERMODEL_FONTMODEL_HPP
