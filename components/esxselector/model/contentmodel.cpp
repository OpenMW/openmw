#include "contentmodel.hpp"
#include "esmfile.hpp"
#include <QDebug>
#include <QDir>
#include <QTextCodec>
#include <components/esm/esmreader.hpp>

EsxModel::ContentModel::ContentModel(QObject *parent) :
    QAbstractTableModel(parent), mEncoding("win1252")
{}

void EsxModel::ContentModel::setEncoding(const QString &encoding)
{
    mEncoding = encoding;
}

int EsxModel::ContentModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 1;
}
/*
QModelIndex EsxModel::ContentModel::parent(const QModelIndex &child) const
{
    if(!child.isValid())
        return 0;

    return child.parent();
}

QModelIndex EsxModel::ContentModel::index(int row, int column, const QModelIndex &parent) const
{

}
*/
int EsxModel::ContentModel::rowCount(const QModelIndex &parent) const
{
    if(parent.isValid())
        return 0;

    return mFiles.size();
}

QVariant EsxModel::ContentModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= mFiles.size())
        return QVariant();

    EsmFile *file = item(index.row());

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
            return QString("%1 kB").arg(int((file->size() + 1023) / 1024));
        case 3:
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

        return QVariant();
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
        case 4:
        case 5:
            return Qt::AlignRight + Qt::AlignVCenter;
        default:
            return Qt::AlignLeft + Qt::AlignVCenter;
        }
        return QVariant();
    }

    case Qt::ToolTipRole:
    {
        if (column != 0)
            return QVariant();

        if (file->version() == 0.0f)
            return QVariant(); // Data not set

        return  QString("<b>Author:</b> %1<br/> \
                        <b>Version:</b> %2<br/> \
                        <br/><b>Description:</b><br/>%3<br/> \
                        <br/><b>Dependencies: </b>%4<br/>")
                        .arg(file->author())
                        .arg(QString::number(file->version()))
                        .arg(file->description())
                        .arg(file->masters().join(", "));
    }

    case Qt::UserRole:
    {
        if (file->masters().size() == 0)
            return "game";
        else
            return "addon";
    }

    default:
        return QVariant();
    }
}

Qt::ItemFlags EsxModel::ContentModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    EsmFile *file = item(index.row());

    if (!file)
        return Qt::NoItemFlags;

    Qt::ItemFlags dragDropFlags = Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    Qt::ItemFlags checkFlags = Qt::ItemIsUserCheckable;
    Qt::ItemFlags defaultFlags = Qt::ItemIsDropEnabled | Qt::ItemIsSelectable;

    if (canBeChecked(file))
        return Qt::ItemIsEnabled | dragDropFlags | checkFlags | defaultFlags;
    else
        return defaultFlags;
}

bool EsxModel::ContentModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole)
    {
        QString fname = value.value<QString>();
        mFiles.replace(index.row(), findItem(fname));
        emit dataChanged(index, index);
        return true;
    }

    return false;
}

bool EsxModel::ContentModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    beginInsertRows(parent, position, position+rows-1);

    for (int row = 0; row < rows; ++row)
        mFiles.insert(position, new EsmFile);

    endInsertRows();
    return true;
}

bool EsxModel::ContentModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    beginRemoveRows(parent, position, position+rows-1);

    for (int row = 0; row < rows; ++row)
        mFiles.removeAt(position);

    endRemoveRows();
    emit dataChanged(index(0,0,parent), index(rowCount()-1, 0, parent));
    return true;
}

Qt::DropActions EsxModel::ContentModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

QStringList EsxModel::ContentModel::mimeTypes() const
{
    QStringList types;
    types << "application/omwcontent";
    return types;
}

QMimeData *EsxModel::ContentModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    QByteArray encodedData;

    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    foreach (const QModelIndex &index, indexes)
    {
        if (index.isValid())
        {
            QString text = data(index, Qt::DisplayRole).toString();
            stream << text;
        }
    }

    mimeData->setData("application/omwcontent", encodedData);
    return mimeData;
}

bool EsxModel::ContentModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (action == Qt::IgnoreAction)
        return true;

    if (!data->hasFormat("application/omwcontent"))
        return false;

    if (column > 0)
        return false;

    int beginRow;

    if (row != -1)
        beginRow = row;
    else if (parent.isValid())
        beginRow = parent.row();
    else
        beginRow = rowCount();

    QByteArray encodedData = data->data("application/omwcontent");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    QStringList newItems;
    int rows = 0;

    while (!stream.atEnd())
    {
        QString text;
        stream >> text;
        newItems << text;
        ++rows;
    }

    insertRows(beginRow, rows, QModelIndex());

    foreach (const QString &text, newItems)
    {
        QModelIndex idx = index(beginRow, 0, QModelIndex());
        setData(idx, text);
        beginRow++;
    }

    return true;
}

void EsxModel::ContentModel::addFile(EsmFile *file)
{
    emit beginInsertRows(QModelIndex(), mFiles.count(), mFiles.count());
    mFiles.append(file);
    emit endInsertRows();
}

void EsxModel::ContentModel::addFiles(const QString &path)
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

EsxModel::EsmFile* EsxModel::ContentModel::findItem(const QString &name)
{
    for (int i = 0; i < mFiles.size(); ++i)
    {
        if (name == item(i)->fileName())
            return item(i);
    }

    // Not found
    return 0;
}

EsxModel::EsmFile* EsxModel::ContentModel::item(int row) const
{
    if (row >= 0 && row < mFiles.count())
        return mFiles.at(row);

    return 0;
}

QModelIndex EsxModel::ContentModel::indexFromItem(EsmFile *item) const
{
    if (item)
        //return createIndex(mFiles.indexOf(item), 0);
        return index(mFiles.indexOf(item),0);

    return QModelIndex();
}

Qt::CheckState EsxModel::ContentModel::checkState(const QModelIndex &index)
{
    return mCheckStates[item(index.row())->fileName()];
}

void EsxModel::ContentModel::setCheckState(const QModelIndex &index, Qt::CheckState state)
{
    if (!index.isValid())
        return;

    QString name = item(index.row())->fileName();
    mCheckStates[name] = state;

    // Force a redraw of the view since unchecking one item can affect another
    QModelIndex firstIndex = indexFromItem(mFiles.first());
    QModelIndex lastIndex = indexFromItem(mFiles.last());

    emit dataChanged(firstIndex, lastIndex);
    //emit checkedItemsChanged(checkedItems());

}

bool EsxModel::ContentModel::canBeChecked(const EsmFile *file) const
{
    //element can be checked if all its dependencies are
    foreach (const QString &master, file->masters())
    {
        if (!mCheckStates.contains(master) || mCheckStates[master] != Qt::Checked)
            return false;
    }
    return true;
}

EsxModel::ContentFileList EsxModel::ContentModel::checkedItems() const
{
    ContentFileList list;

    for (int i = 0; i < mFiles.size(); ++i)
    {
        EsmFile *file = item(i);

        // Only add the items that are in the checked list and available
        if (mCheckStates[file->fileName()] == Qt::Checked && canBeChecked(file))
            list << file;
    }

    return list;
}

void EsxModel::ContentModel::uncheckAll()
{
    emit layoutAboutToBeChanged();
    mCheckStates.clear();
    emit layoutChanged();
}
