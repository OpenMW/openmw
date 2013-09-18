#include <QTextDecoder>
#include <QTextCodec>
#include <QFileInfo>
#include <QDir>
#include <QtAlgorithms>

#include <stdexcept>

#include <components/esm/esmreader.hpp>

#include "esmfile.hpp"

#include "datafilesmodel.hpp"

#include <QDebug>

EsxModel::DataFilesModel::DataFilesModel(QObject *parent) :
    QAbstractTableModel(parent)
{
    mEncoding = QString("win1252");
}

EsxModel::DataFilesModel::~DataFilesModel()
{
}

int EsxModel::DataFilesModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : mFiles.count();
}

int EsxModel::DataFilesModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 1;
}

const EsxModel::EsmFile* EsxModel::DataFilesModel::findItem(const QString &name)
{
    for (int i = 0; i < mFiles.size(); ++i)
    {
        const EsmFile *file = item(i);

        if (name == file->fileName())
            return file;
    }

    return 0;
}

const EsxModel::EsmFile* EsxModel::DataFilesModel::item(int row) const
{
    if (row >= 0 && row < mFiles.count())
        return mFiles.at(row);

    return 0;
}

Qt::ItemFlags EsxModel::DataFilesModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    const EsmFile *file = item(index.row());

    if (!file)
        return Qt::NoItemFlags;

    Qt::ItemFlags dragDropFlags = Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    Qt::ItemFlags checkFlags = Qt::ItemIsUserCheckable | Qt::ItemIsEditable;
    Qt::ItemFlags defaultFlags = Qt::ItemIsDropEnabled | Qt::ItemIsSelectable;

    if (canBeChecked(file))
        return defaultFlags | dragDropFlags | checkFlags | Qt::ItemIsEnabled;
    else
        return defaultFlags;
}

QVariant EsxModel::DataFilesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= mFiles.size())
        return QVariant();

    const EsmFile *file = item(index.row());

    if (!file)
        return QVariant();

    const int column = index.column();

    switch (role)
    {
    case Qt::EditRole:
    case Qt::DisplayRole:
    {

        switch (column)
        {
        case 0:
            return file->fileName();
        case 1:
            return file->author();
        case 2:
            return file->modified().toString(Qt::ISODate);
        case 3:
            return file->version();
        case 4:
            return file->path();
        case 5:
            return file->masters().join(", ");
        case 6:
            return file->description();
        }
        break;
    }

    case Qt::TextAlignmentRole:
    {
        switch (column)
        {
        case 0:
        case 1:
            return Qt::AlignLeft + Qt::AlignVCenter;
        case 2:
        case 3:
             return Qt::AlignRight + Qt::AlignVCenter;
        default:
            return Qt::AlignLeft + Qt::AlignVCenter;
        }
        break;
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
        break;
    }

    case Qt::UserRole:
    {
        if (file->masters().size() == 0)
            return "game";
        else
            return "addon";

        break;
    }

    case Qt::UserRole + 1:
        //return check state here
        break;

    default:
        return QVariant();
        break;
    }

}

bool EsxModel::DataFilesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    switch (role)
    {
        case Qt::EditRole:
        {
            const EsmFile *file = item(index.row());

            // iterate loop to repopulate file pointer with data in string list.
            QStringList list = value.toStringList();
            for (int i = 0; i <999; ++i)
            {
                file->setProperty(i, value.at(i));
            }

            //populate master list here (emit data changed for each master and
            //each item (other than the dropped item) which share each of the masters
            file->masters().append(masterList);

            emit dataChanged(index, index);
            return true;
        }
            break;

        case Qt::UserRole + 1:
        {
            EsmFile *file = item(index.row());
            //set file's checkstate to the passed checkstate
            emit dataChanged(index, index);

            for (int i = 0; i < mFiles.size(); ++i)
                if (mFiles.at(i)->getMasters().contains(file->fileName()))
                    emit dataChanged(QAbstractTableModel::index(i,0), QAbstractTableModel::index(i,0));

            return true;
        }
            break;

        case Qt::CheckStateRole:
        {
            EsmFile *file = item(index.row());

            if ((value.toInt() == Qt::Checked) && !file->isChecked())
                file->setChecked(true);
            else if (value.toInt() == Qt::Checked && file->isChecked())
                file->setChecked(false);
            else if (value.toInt() == Qt::UnChecked)
                file->setChecked(false);

            emit dataChanged(index, index);

            return true;
        }
            break;
    }
    return false;
}

bool EsxModel::DataFilesModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid())
        return false;

    beginInsertRows(QModelIndex(),row, row+count-1);
    {
        for (int i = 0; i < count; ++i)
            mFiles.insert(row, new EsmFile());
    } endInsertRows();

    return true;
}

bool EsxModel::DataFilesModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid())
        return false;

    beginRemoveRows(QModelIndex(), row, row+count-1);
    {
        for (int i = 0; i < count; ++i)
            delete mFiles.takeAt(row);
    } endRemoveRows();

    return true;
}

Qt::DropActions EsxModel::DataFilesModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

QStringList EsxModel::DataFilesModel::mimeTypes() const
{
    QStringList types;
    types << "application/omwcontent";
    return types;
}

QMimeData *EsxModel::DataFilesModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    QByteArray encodedData;

    QDataStream stream (&encodedData, QIODevice::WriteOnly);

    foreach (const QModelIndex &index, indexes)
    {
        if (index.isValid())
        {
            EsmFile *file = item (index.row());

            for (int i = 0; i < file->propertyCount(); ++i)
            stream << data(index, Qt::DisplayRole).toString();

            EsmFile *file = item(index.row());
            stream << file->getMasters();
        }
    }

    mimeData->setData("application/omwcontent", encodedData);

    return mimeData;
}

bool EsxModel::DataFilesModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (action == Qt::IgnoreAction)
        return true;

    if (action != Qt::MoveAction)
        return false;

    if (!data->hasFormat("application/omwcontent"))
        return false;

    int dropRow = row;

    if (dropRow == -1)
    {
        if (parent.isValid())
            dropRow = parent.row();
        else
            dropRow = rowCount(QModelIndex());
    }

        if (parent.isValid())
                qDebug() << "parent: " << parent.data().toString();
        qDebug() << "dragged file: " << (qobject_cast<const EsmFile *>(data))->fileName();
//    qDebug() << "inserting file: " << droppedfile->fileName() << " ahead of " << file->fileName();
    insertRows (dropRow, 1, QModelIndex());


    const EsmFile *draggedFile = qobject_cast<const EsmFile *>(data);

    int dragRow = -1;

    for (int i = 0; i < mFiles.size(); ++i)
        if (draggedFile->fileName() == mFiles.at(i)->fileName())
        {
            dragRow = i;
            break;
        }

    for (int i = 0; i < mFiles.count(); ++i)
    {
        qDebug() << "index: " << i << "file: " << item(i)->fileName();
        qDebug() << mFiles.at(i)->fileName();
    }

    qDebug() << "drop row: " << dropRow << "; drag row: " << dragRow;
//    const EsmFile *file = qobject_cast<const EsmFile *>(data);
  //  int index = mFiles.indexOf(file);
    //qDebug() << "file name: " << file->fileName() << "; index: " << index;
    mFiles.swap(dropRow, dragRow);
    //setData(index(startRow, 0), varFile);
        emit dataChanged(index(0,0), index(rowCount(),0));
    return true;
}

void EsxModel::DataFilesModel::setEncoding(const QString &encoding)
{
    mEncoding = encoding;
}

void EsxModel::DataFilesModel::setCheckState(const QModelIndex &index, Qt::CheckState state)
{
    if (!index.isValid())
        return;

    QString name = item(index.row())->fileName();
    mCheckStates[name] = state;

    // Force a redraw of the view since unchecking one item can affect another
    QModelIndex firstIndex = indexFromItem(mFiles.first());
    QModelIndex lastIndex = indexFromItem(mFiles.last());

    emit dataChanged(firstIndex, lastIndex);
    emit checkedItemsChanged(checkedItems());

}

Qt::CheckState EsxModel::DataFilesModel::checkState(const QModelIndex &index)
{
    return mCheckStates[item(index.row())->fileName()];
}

bool EsxModel::DataFilesModel::moveRow(int oldrow, int row, const QModelIndex &parent)
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

QVariant EsxModel::DataFilesModel::headerData(int section, Qt::Orientation orientation, int role) const
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
    }
    return QVariant();
}

//!!!!!!!!!!!!!!!!!!!!!!!
bool lessThanEsmFile(const EsxModel::EsmFile *e1, const EsxModel::EsmFile *e2)
{
    //Masters first then alphabetically
    if (e1->fileName().endsWith(".esm") && !e2->fileName().endsWith(".esm"))
        return true;
    if (!e1->fileName().endsWith(".esm") && e2->fileName().endsWith(".esm"))
        return false;

    return e1->fileName().toLower() < e2->fileName().toLower();
}
//!!!!!!!!!!!!!!!!!!!!!!!
bool lessThanDate(const EsxModel::EsmFile *e1, const EsxModel::EsmFile *e2)
{
    if (e1->modified().toString(Qt::ISODate) < e2->modified().toString(Qt::ISODate))
        return true;
    else
        return false;
}
//!!!!!!!!!!!!!!!!!!!!!!!
void EsxModel::DataFilesModel::sort(int column, Qt::SortOrder order)
{
    emit layoutAboutToBeChanged();

    if (column == 3) {
        qSort(mFiles.begin(), mFiles.end(), lessThanDate);
    } else {
        qSort(mFiles.begin(), mFiles.end(), lessThanEsmFile);
    }

    emit layoutChanged();
}

void EsxModel::DataFilesModel::addFile(const EsmFile *file)
{
    emit beginInsertRows(QModelIndex(), mFiles.count(), mFiles.count());
    mFiles.append(file);
    emit endInsertRows();
}

void EsxModel::DataFilesModel::addFiles(const QString &path)
{
    QDir dir(path);
    QStringList filters;
    filters << "*.esp" << "*.esm" << "*.omwgame" << "*.omwaddon";
    dir.setNameFilters(filters);

    // Create a decoder for non-latin characters in esx metadata
    QTextCodec *codec;

    if (mEncoding == QLatin1String("win1252")) {
        codec = QTextCodec::codecForName("windows-1252");
    } else if (mEncoding == QLatin1String("win1251")) {
        codec = QTextCodec::codecForName("windows-1251");
    } else if (mEncoding == QLatin1String("win1250")) {
        codec = QTextCodec::codecForName("windows-1250");
    } else {
        return; // This should never happen;
    }

    QTextDecoder *decoder = codec->makeDecoder();

    foreach (const QString &path, dir.entryList()) {
        QFileInfo info(dir.absoluteFilePath(path));
        EsmFile *file = new EsmFile(path);

        try {
            ESM::ESMReader fileReader;
            ToUTF8::Utf8Encoder encoder(ToUTF8::calculateEncoding(mEncoding.toStdString()));
            fileReader.setEncoder(&encoder);
            fileReader.open(dir.absoluteFilePath(path).toStdString());

            std::vector<ESM::Header::MasterData> mlist = fileReader.getMasters();

            QStringList masters;

            for (unsigned int i = 0; i < mlist.size(); ++i) {
                QString master = QString::fromStdString(mlist[i].name);
                masters.append(master);
            }

            file->setAuthor(decoder->toUnicode(fileReader.getAuthor().c_str()));
            file->setSize(info.size());
            file->setDates(info.lastModified(), info.lastRead());
            file->setVersion(fileReader.getFVer());
            file->setPath(info.absoluteFilePath());
            file->setMasters(masters);
            file->setDescription(decoder->toUnicode(fileReader.getDesc().c_str()));


            // Put the file in the table
            if (findItem(path) == 0)
                addFile(file);

        } catch(std::runtime_error &e) {
            // An error occurred while reading the .esp
            qWarning() << "Error reading addon file: " << e.what();
            continue;
        }

    }

    delete decoder;
}

QModelIndex EsxModel::DataFilesModel::indexFromItem(const EsmFile *item) const
{
    if (item)
        //return createIndex(mFiles.indexOf(item), 0);
        return index(mFiles.indexOf(item),0);

    return QModelIndex();
}

EsxModel::EsmFileList EsxModel::DataFilesModel::checkedItems()
{
    EsmFileList list;

    EsmFileList::ConstIterator it;
    EsmFileList::ConstIterator itEnd = mFiles.constEnd();

    for (it = mFiles.constBegin(); it != itEnd; ++it)
    {
        // Only add the items that are in the checked list and available
        if (mCheckStates[(*it)->fileName()] == Qt::Checked && canBeChecked(*it))
            list << (*it);
    }

    return list;
}

QStringList EsxModel::DataFilesModel::checkedItemsPaths()
{
    QStringList list;

    EsmFileList::ConstIterator it;
    EsmFileList::ConstIterator itEnd = mFiles.constEnd();

    int i = 0;
    for (it = mFiles.constBegin(); it != itEnd; ++it) {
        const EsmFile *file = item(i);
        ++i;

        if (mCheckStates[file->fileName()] == Qt::Checked && canBeChecked(file))
            list << file->path();
    }

    return list;
}
void EsxModel::DataFilesModel::uncheckAll()
{
    emit layoutAboutToBeChanged();
    mCheckStates.clear();
    emit layoutChanged();
}

/*
EsxModel::EsmFileList EsxModel::DataFilesModel::uncheckedItems()
{
    EsmFileList list;
    EsmFileList checked = checkedItems();

    EsmFileList::ConstIterator it;

    for (it = mFiles.constBegin(); it != mFiles.constEnd(); ++it)
    {
        const EsmFile *file = *it;

        // Add the items that are not in the checked list
        if (!checked.contains(file))
            list << file;
    }

    return list;
}
*/
bool EsxModel::DataFilesModel::canBeChecked(const EsmFile *file) const
{
    //element can be checked if all its dependencies are
    foreach (const QString &master, file->masters())
    {
        if (!mCheckStates.contains(master) || mCheckStates[master] != Qt::Checked)
            return false;
    }
    return true;
}
