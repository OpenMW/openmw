#include <QDebug>
#include <QFileInfo>
#include <QDir>

#include <components/esm/esmreader.hpp>

#include "esm/esmfile.hpp"

#include "../utils/naturalsort.hpp"

#include "datafilesmodel.hpp"

DataFilesModel::DataFilesModel(QObject *parent) :
    QAbstractTableModel(parent)
{
}

DataFilesModel::~DataFilesModel()
{
}

void DataFilesModel::setCheckState(const QModelIndex &index, Qt::CheckState state)
{
    setData(index, state, Qt::CheckStateRole);
}

Qt::CheckState DataFilesModel::checkState(const QModelIndex &index)
{
    EsmFile *file = item(index.row());
    return mCheckStates[file->fileName()];
}

int DataFilesModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 9;
}

int DataFilesModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : mFiles.count();
}


bool DataFilesModel::moveRow(int oldrow, int row, const QModelIndex &parent)
{
    if (oldrow < 0 || row < 0 || oldrow == row)
        return false;

    emit layoutAboutToBeChanged();
    //emit beginMoveRows(parent, oldrow, oldrow, parent, row);
    mFiles.swap(oldrow, row);
    //emit endInsertRows();
    emit layoutChanged();

    return true;
}

QVariant DataFilesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

//    if (index.row() < 0 || index.row() >= mPlugins.size())
//        return QVariant();

    EsmFile *file = mFiles.at(index.row());

    const int column = index.column();

    switch (role) {
    case Qt::DisplayRole: {

        switch (column) {
        case 0:
            return file->fileName();
        case 1:
            return file->author();
        case 2:
            return QString("%1 kB").arg(int((file->size() + 1023) / 1024));
        case 3:
            //return file->modified().toString(Qt::TextDate);
            return file->modified().toString(Qt::ISODate);
        case 4:
            return file->accessed().toString(Qt::TextDate);
        case 5:
            return file->version();
        case 6:
            return file->path();
        case 7:
            return file->masters().join(", ");
        case 8:
            return file->description();
        }
    }

    case Qt::TextAlignmentRole: {
        switch (column) {
        case 0:
        case 1:
            return Qt::AlignLeft + Qt::AlignVCenter;
        case 2:
        case 3:
        case 4:
        case 5:
            return Qt::AlignRight + Qt::AlignVCenter;
        default:
            return Qt::AlignLeft + Qt::AlignVCenter;
        }
    }

    case Qt::CheckStateRole: {
        if (column != 0)
            return QVariant();
        return mCheckStates[file->fileName()];
    }
    case Qt::ToolTipRole:
    {
        if (column != 0)
            return QVariant();

        if (file->version() == 0.0f)
            return QVariant(); // Data not set

        QString tooltip =
                QString("<b>Author:</b> %1<br/> \
                        <b>Version:</b> %2<br/> \
                        <br/><b>Description:</b><br/>%3<br/> \
                        <br/><b>Dependencies: </b>%4<br/>")
                        .arg(file->author())
                        .arg(QString::number(file->version()))
                        .arg(file->description())
                        .arg(file->masters().join(", "));


        return tooltip;

    }
    default:
        return QVariant();
    }

}

Qt::ItemFlags DataFilesModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    EsmFile *file = mFiles.at(index.row());

    if (mAvailableFiles.contains(file->fileName())) {
        if (index.column() == 0) {
            return Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        } else {
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        }
    } else {
        if (index.column() == 0) {
            return Qt::ItemIsUserCheckable | Qt::ItemIsSelectable;
        } else {
            return Qt::NoItemFlags | Qt::ItemIsSelectable;
        }
    }

}

QVariant DataFilesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
        case 0: return tr("Name");
        case 1: return tr("Author");
        case 2: return tr("Size");
        case 3: return tr("Modified");
        case 4: return tr("Accessed");
        case 5: return tr("Version");
        case 6: return tr("Path");
        case 7: return tr("Masters");
        case 8: return tr("Description");
        }
    } else {
        // Show row numbers
        return ++section;
    }

    return QVariant();
}

bool DataFilesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
            return false;

    if (role == Qt::CheckStateRole) {

        emit layoutAboutToBeChanged();

        QString name = item(index.row())->fileName();
        mCheckStates[name] = static_cast<Qt::CheckState>(value.toInt());

        emit checkedItemsChanged(checkedItems(), uncheckedItems());
        emit layoutChanged();
        return true;
    }

    return false;
}

void DataFilesModel::sort(int column, Qt::SortOrder order)
{
    // TODO: Make this more efficient
    emit layoutAboutToBeChanged();


    QMap<QString, QString> timestamps;

    QList<EsmFile *>::ConstIterator it;
    QList<EsmFile *>::ConstIterator itEnd = mFiles.constEnd();

    // Make a list of files sorted by timestamp
    int i = 0;
    for (it = mFiles.constBegin(); it != itEnd; ++it) {
        EsmFile *file = item(i);
        ++i;
        timestamps[file->modified().toString(Qt::ISODate)] = file->fileName();
    }


    // Now sort the actual list of files by timestamp
    i = 0;
    QMapIterator<QString, QString> ti(timestamps);
    while (ti.hasNext()) {
        ti.next();
        i++;

        QModelIndex index = indexFromItem(findItem(ti.value()));

        if (index.isValid()) {
            mFiles.swap(index.row(), i);
        }

    }

    emit layoutChanged();
}

void DataFilesModel::addFile(EsmFile *file)
{
    emit beginInsertRows(QModelIndex(), mFiles.count(), mFiles.count());
    mFiles.append(file);
    emit endInsertRows();
}

void DataFilesModel::addMasters(const QString &path)
{
    QDir dir(path);
    dir.setNameFilters(QStringList(QLatin1String("*.esp")));

    // Read the dependencies from the plugins
    foreach (const QString &path, dir.entryList()) {
        QFileInfo info(dir.absoluteFilePath(path));

        try {
            ESM::ESMReader fileReader;
            fileReader.setEncoding(std::string("win1252"));
            fileReader.open(dir.absoluteFilePath(path).toStdString());

            ESM::ESMReader::MasterList mlist = fileReader.getMasters();

            for (unsigned int i = 0; i < mlist.size(); ++i) {
                QString master = QString::fromStdString(mlist[i].name);

                // Add the plugin to the internal dependency map
                mDependencies[master].append(path);

                // Don't add esps
                if (master.endsWith(".esp", Qt::CaseInsensitive))
                    continue;

                EsmFile *file = new EsmFile(master);

                // Add the master to the table
                if (findItem(master) == 0)
                    addFile(file);


            }

        } catch(std::runtime_error &e) {
            // An error occurred while reading the .esp
            continue;
        }
    }

    // See if the masters actually exist in the filesystem
    dir.setNameFilters(QStringList(QLatin1String("*.esm")));

    foreach (const QString &path, dir.entryList()) {

        if (findItem(path) == 0) {
            EsmFile *file = new EsmFile(path);
            addFile(file);
        }

        // Make the master selectable
        mAvailableFiles.append(path);
    }
}

void DataFilesModel::addPlugins(const QString &path)
{
    QDir dir(path);
    dir.setNameFilters(QStringList(QLatin1String("*.esp")));

    foreach (const QString &path, dir.entryList()) {
        QFileInfo info(dir.absoluteFilePath(path));
        EsmFile *file = new EsmFile(path);

        try {
            ESM::ESMReader fileReader;
            fileReader.setEncoding(std::string("win1252"));
            fileReader.open(dir.absoluteFilePath(path).toStdString());

            ESM::ESMReader::MasterList mlist = fileReader.getMasters();
            QStringList masters;

            for (unsigned int i = 0; i < mlist.size(); ++i) {
                QString master = QString::fromStdString(mlist[i].name);
                masters.append(master);

                // Add the plugin to the internal dependency map
                mDependencies[master].append(path);
            }

            file->setAuthor(QString::fromStdString(fileReader.getAuthor()));
            file->setSize(info.size());
            file->setDates(info.lastModified(), info.lastRead());
            file->setVersion(fileReader.getFVer());
            file->setPath(info.absoluteFilePath());
            file->setMasters(masters);
            file->setDescription(QString::fromStdString(fileReader.getDesc()));


            // Put the file in the table
            addFile(file);
        } catch(std::runtime_error &e) {
            // An error occurred while reading the .esp
            continue;
        }

    }
}

QModelIndex DataFilesModel::indexFromItem(EsmFile *item) const
{
    if (item)
        return createIndex(mFiles.indexOf(item), 0);

    return QModelIndex();
}

EsmFile* DataFilesModel::findItem(const QString &name)
{
    QList<EsmFile *>::ConstIterator it;
    QList<EsmFile *>::ConstIterator itEnd = mFiles.constEnd();

    int i = 0;
    for (it = mFiles.constBegin(); it != itEnd; ++it) {
        EsmFile *file = item(i);
        ++i;

        if (name == file->fileName())
            return file;
    }

    // Not found
    return 0;
}

EsmFile* DataFilesModel::item(int row)
{
    if (row >= 0 && row < mFiles.count())
        return mFiles.at(row);
    else
        return 0;
}

QStringList DataFilesModel::checkedItems()
{
    QStringList list;

    QList<EsmFile *>::ConstIterator it;
    QList<EsmFile *>::ConstIterator itEnd = mFiles.constEnd();

    int i = 0;
    for (it = mFiles.constBegin(); it != itEnd; ++it) {
        EsmFile *file = item(i);
        ++i;

        QString name = file->fileName();

        // Only add the items that are in the checked list and available
        if (mCheckStates[name] == Qt::Checked && mAvailableFiles.contains(name))
            list << name;
    }

    return list;
}

void DataFilesModel::uncheckAll()
{
    emit layoutAboutToBeChanged();
    mCheckStates.clear();
    emit layoutChanged();
}

QStringList DataFilesModel::uncheckedItems()
{
    QStringList list;
    QStringList checked = checkedItems();

    QList<EsmFile *>::ConstIterator it;
    QList<EsmFile *>::ConstIterator itEnd = mFiles.constEnd();

    int i = 0;
    for (it = mFiles.constBegin(); it != itEnd; ++it) {
        EsmFile *file = item(i);
        ++i;

        // Add the items that are not in the checked list
        if (!checked.contains(file->fileName()))
            list << file->fileName();
    }

    return list;
}

void DataFilesModel::slotcheckedItemsChanged(const QStringList &checkedItems, const QStringList &unCheckedItems)
{
    emit layoutAboutToBeChanged();

    QStringList list;

    foreach (const QString &file, checkedItems) {
        list << mDependencies[file];
    }

    foreach (const QString &file, unCheckedItems) {
        foreach (const QString &remove, mDependencies[file]) {
            list.removeAll(remove);
        }
    }

    mAvailableFiles.clear();
    mAvailableFiles.append(list);

    emit layoutChanged();
}
