#include "datafilesmodel.h"

DataFilesModel::DataFilesModel(QObject *parent)
    : QFileSystemModel(parent)
{
}

QVariant DataFilesModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && role == Qt::DecorationRole) {
        if (index.column() == 2) {
            QFileIconProvider fip;
            QIcon fileIcon = fip.icon(fileInfo(index));
            return fileIcon;
        }
        else {
            return QIcon();
        }

    }

    if (index.isValid() && role == Qt::DisplayRole) {
        if (index.column() == 2) {
           //qDebug() << index.data(Qt::DisplayRole);
           if (fileInfo(index).suffix().toLower() == "esp") {
               return QString("Plugin File");
           }
           else if (fileInfo(index).suffix().toLower() == "esm") {
               return QString("Master File");

           }
        }
    }

    if (index.isValid() && role == Qt::CheckStateRole && index.column() == 0) {
        // Put a checkbox in the first column
        if (index.isValid())

        if (checkedItems.contains(filePath(index))) {
//        if (checkedItems.contains(index)) {
            return Qt::Checked;
        }
        else {
            return Qt::Unchecked;
        }
    }
    return QFileSystemModel::data(index, role);
}

bool DataFilesModel::setData(const QModelIndex& index, const QVariant& value, int role)
{

    if (index.isValid() && role == Qt::CheckStateRole && index.column() == 0) {
//        QPersistentModelIndex pindex(index);

  //      qDebug() << pindex;

        if (value == Qt::Checked) {
            //checkedItems.insert(pindex);
            checkedItems.append(filePath(index));
        } else {
//            checkedItems.remove(pindex);
            checkedItems.removeAll(filePath(index));
        }

        emit dataChanged(index, index);
        return true;
    }

    return QFileSystemModel::setData(index, value, role);
}



Qt::ItemFlags DataFilesModel::flags(const QModelIndex &index) const
{
    return QFileSystemModel::flags(index) | Qt::ItemIsUserCheckable;
}



/*QVariant DataFilesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (role) {
    case Qt::DecorationRole:
        if (section == 0) {
            // ### TODO oh man this is ugly and doesn't even work all the way!
            // it is still 2 pixels off
            QImage pixmap(16, 1, QImage::Format_Mono);
            pixmap.fill(0);
            pixmap.setAlphaChannel(pixmap.createAlphaMask());
            return pixmap;
        }
        break;
    case Qt::TextAlignmentRole:
        return Qt::AlignLeft;
    }

    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractItemModel::headerData(section, orientation, role);

    QString returnValue;
    switch (section) {
    case 0: returnValue = tr("TES3 File");
            break;
    case 1: returnValue = tr("Size");
            break;
    case 2: returnValue = tr("Status");
           break;
    case 3: returnValue = tr("Date Modified");
            break;
    default: return QVariant();
    }
    return returnValue;
}
*/

/*test
void DataFilesModel::setCheckedItems(const QStringList &gameFiles)
{
    qDebug() << "test";
    qDebug() << gameFiles.join("lol");


}*/

/*void DataFilesModel::unCheckAll()
{
    checkedItems.clear();
//    data();
    emit dataChanged(QModelIndex(), QModelIndex());
    submit();
}*/

const QStringList DataFilesModel::getCheckedItems()
//const QList<QPersistentModelIndex> DataFilesModel::getCheckedItems()
//const QSet<QPersistentModelIndex> DataFilesModel::getCheckedItems()
{
    return checkedItems;
}

/*void DataFilesModel::sort(int column, Qt::SortOrder order)
{
    qDebug() << "Sort!";
}*/
