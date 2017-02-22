#include <qmessagebox.h>
#include "ServerModel.hpp"
#include <qdebug.h>

ServerModel::ServerModel(QObject *parent) : QAbstractTableModel(parent)
{
}

ServerModel::~ServerModel()
{

}

/*QHash<int, QByteArray> ServerModel::roleNames() const
{
    return roles;
}*/

QVariant ServerModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (index.row() < 0 || index.row() > myData.size())
        return QVariant();

    const ServerData &sd = myData.at(index.row());

    if(role == Qt::DisplayRole)
    {
        QVariant var;
        switch (index.column())
        {
            case ServerData::ADDR:
                var = sd.addr;
                break;
            case ServerData::PASSW:
                var = sd.needPassw ? "Yes" : "No";
                break;
            case ServerData::VERSION:
                var = sd.version;
                break;
            case ServerData::PLAYERS:
                var = sd.players;
                break;
            case ServerData::MAX_PLAYERS:
                var = sd.maxPlayers;
                break;
            case ServerData::HOSTNAME:
                var = sd.hostName;
                break;
            case ServerData::PING:
                var = sd.ping;
                break;
            case ServerData::MODNAME:
                if(sd.modName.isEmpty())
                    var = "default";
                else
                    var = sd.modName;
                break;
        }
        return var;
    }
    return QVariant();
}

QVariant ServerModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    QVariant var;
    if (orientation == Qt::Horizontal)
    {
        if (role == Qt::SizeHintRole)
        {
            /*if(section == ServerData::HOSTNAME)
                var = QSize(200, 25);*/
        }
        else if (role == Qt::DisplayRole)
        {

            switch (section)
            {
                case ServerData::ADDR:
                    var = "Address";
                    break;
                case ServerData::PASSW:
                    var = "Password";
                    break;
                case ServerData::VERSION:
                    var = "Version";
                    break;
                case ServerData::HOSTNAME:
                    var = "Host name";
                    break;
                case ServerData::PLAYERS:
                    var = "Players";
                    break;
                case ServerData::MAX_PLAYERS:
                    var = "Max players";
                    break;
                case ServerData::PING:
                    var = "Ping";
                    break;
                case ServerData::MODNAME:
                    var = "Game mode";
            }
        }
    }
    return var;
}

int ServerModel::rowCount(const QModelIndex &parent) const
{
    return myData.size();
}

int ServerModel::columnCount(const QModelIndex &parent) const
{
    return ServerData::LAST;
}

bool ServerModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole)
    {
        int row = index.row();
        int col = index.column();

        ServerData &sd = myData[row];
        bool ok = true;
        switch(col)
        {
            case ServerData::ADDR:
                sd.addr = value.toString();
                ok = !sd.addr.isEmpty();
                break;
            case ServerData::PASSW:
                sd.needPassw = value.toBool();
                break;
            case ServerData::VERSION:
                sd.version = value.toString();
                ok = !sd.addr.isEmpty();
                break;
            case ServerData::PLAYERS:
                sd.players = value.toInt(&ok);
                break;
            case ServerData::MAX_PLAYERS:
                sd.maxPlayers = value.toInt(&ok);
                break;
            case ServerData::HOSTNAME:
                sd.hostName = value.toString();
                ok = !sd.addr.isEmpty();
                break;
            case ServerData::PING:
                sd.ping = value.toInt(&ok);
                break;
            case ServerData::MODNAME:
                sd.modName = value.toString();
                break;
            default:
                return false;
        }
        if(ok)
            emit(dataChanged(index, index));
        return true;
    }
    return false;
}

bool ServerModel::insertRows(int position, int count, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginInsertRows(QModelIndex(), position, position + count - 1);

    for (int row = 0; row < count; ++row) {
        ServerData sd {"", -1, -1, -1, "", "", false, 0};
        myData.insert(position, sd);
    }

    endInsertRows();
    return true;
}

bool ServerModel::removeRows(int position, int count, const QModelIndex &parent)
{
    if (count == 0)
        return false;

    beginRemoveRows(parent, position, position + count - 1);
    myData.erase(myData.begin()+position, myData.begin() + position + count);
    endRemoveRows();

    return true;
}

QModelIndex ServerModel::index(int row, int column, const QModelIndex &parent) const
{

    QModelIndex index = QAbstractTableModel::index(row, column, parent);
    //qDebug() << "Valid index? " << index.isValid() << "   " << row << "   " << column;
    return index;
}
