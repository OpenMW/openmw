#include "contentmodel.hpp"
#include "esmfile.hpp"

#include <QDir>
#include <QTextCodec>
#include <components/esm/esmreader.hpp>
#include <QDebug>

EsxModel::ContentModel::ContentModel(QObject *parent) :
    QAbstractTableModel(parent),
    mMimeType ("application/omwcontent"),
    mMimeTypes (QStringList() << mMimeType),
    mColumnCount (1),
    mDragDropFlags (Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled),
    mDefaultFlags (Qt::ItemIsDropEnabled | Qt::ItemIsSelectable),
    mDropActions (Qt::CopyAction | Qt::MoveAction)
{
    setEncoding ("win1252");
    uncheckAll();
}

void EsxModel::ContentModel::setEncoding(const QString &encoding)
{
    if (encoding == QLatin1String("win1252"))
        mCodec = QTextCodec::codecForName("windows-1252");

    else if (encoding == QLatin1String("win1251"))
        mCodec = QTextCodec::codecForName("windows-1251");

    else if (encoding == QLatin1String("win1250"))
        mCodec = QTextCodec::codecForName("windows-1250");

    else
        return; // This should never happen;
}

int EsxModel::ContentModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return mColumnCount;
}

int EsxModel::ContentModel::rowCount(const QModelIndex &parent) const
{
    if(parent.isValid())
        return 0;

    return mFiles.size();
}

const EsxModel::EsmFile* EsxModel::ContentModel::item(int row) const
{
    if (row >= 0 && row < mFiles.size())
        return mFiles.at(row);

    return 0;
}

EsxModel::EsmFile *EsxModel::ContentModel::item(int row)
{
    if (row >= 0 && row < mFiles.count())
        return mFiles.at(row);

    return 0;
}
const EsxModel::EsmFile *EsxModel::ContentModel::findItem(const QString &name) const
{
    foreach (const EsmFile *file, mFiles)
    {
        if (name == file->fileName())
            return file;
    }
    return 0;
}

QModelIndex EsxModel::ContentModel::indexFromItem(EsmFile *item) const
{
    if (item)
        return index(mFiles.indexOf(item),0);

    return QModelIndex();
}

Qt::ItemFlags EsxModel::ContentModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    const EsmFile *file = item(index.row());

    if (!file)
        return Qt::NoItemFlags;

    if (canBeChecked(file))
        return Qt::ItemIsEnabled | mDragDropFlags | mDefaultFlags;

    return mDefaultFlags;
}

QVariant EsxModel::ContentModel::data(const QModelIndex &index, int role) const
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
        if (column >=0 && column <=EsmFile::FileProperty_Master)
            return file->fileProperty(static_cast<const EsmFile::FileProperty>(column));

        return QVariant();
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
        return QVariant();
        break;
    }

    case Qt::ToolTipRole:
    {
        if (column != 0)
            return QVariant();

        return file->toolTip();
        break;
    }

    case Qt::CheckStateRole:
    {
        if (!file->isMaster())
            return isChecked(file->fileName());
        break;
    }

    case Qt::UserRole:
    {
        if (file->isMaster())
            return "game";
        else
            return "addon";
    }

    case Qt::UserRole + 1:
        return isChecked(file->fileName());
        break;
    }
    return QVariant();
}

bool EsxModel::ContentModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!index.isValid())
        return false;

    EsmFile *file = item(index.row());
    QString fileName = file->fileName();
    bool success = false;

    switch(role)
    {
        case Qt::EditRole:
        {
            QStringList list = value.toStringList();

            for (int i = 0; i < EsmFile::FileProperty_Master; i++)
                file->setFileProperty(static_cast<EsmFile::FileProperty>(i), list.at(i));

            for (int i = EsmFile::FileProperty_Master; i < list.size(); i++)
                file->setFileProperty (EsmFile::FileProperty_Master, list.at(i));

            emit dataChanged(index, index);

            success = true;
        }
        break;

        case Qt::UserRole+1:
        {
            setCheckState(fileName, value.toBool());

            emit dataChanged(index, index);

            foreach (EsmFile *file, mFiles)
            {
                if (file->masters().contains(fileName))
                {
                    QModelIndex idx = indexFromItem(file);
                    emit dataChanged(idx, idx);
                }
            }
            success = true;
        }
        break;

        case Qt::CheckStateRole:
        {
            int checkValue = value.toInt();

            if ((checkValue==Qt::Checked) && !isChecked(fileName))
                setCheckState(fileName, true);
            else if ((checkValue == Qt::Checked) && isChecked (fileName))
                setCheckState(fileName, false);
            else if (checkValue == Qt::Unchecked)
                setCheckState(fileName, false);

            emit dataChanged(index, index);

            success =  true;
        }
        break;
    }

    return success;
}

bool EsxModel::ContentModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    if (parent.isValid())
        return false;

    beginInsertRows(parent, position, position+rows-1);
    {
        for (int row = 0; row < rows; ++row)
            mFiles.insert(position, new EsmFile);

    } endInsertRows();

    return true;
}

bool EsxModel::ContentModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    if (parent.isValid())
        return false;

    beginRemoveRows(parent, position, position+rows-1);
    {
        for (int row = 0; row < rows; ++row)
            delete mFiles.takeAt(position);

    } endRemoveRows();

    return true;
}

Qt::DropActions EsxModel::ContentModel::supportedDropActions() const
{
    return mDropActions;
}

QStringList EsxModel::ContentModel::mimeTypes() const
{
    return mMimeTypes;
}

QMimeData *EsxModel::ContentModel::mimeData(const QModelIndexList &indexes) const
{
    QByteArray encodedData;

    foreach (const QModelIndex &index, indexes)
    {
        if (!index.isValid())
            continue;

        encodedData.append(item(index.row())->encodedData());
    }

    QMimeData *mimeData = new QMimeData();
    mimeData->setData(mMimeType, encodedData);

    return mimeData;
}

bool EsxModel::ContentModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (action == Qt::IgnoreAction)
        return true;

    if (column > 0)
        return false;

    if (!data->hasFormat(mMimeType))
        return false;

    int beginRow = rowCount();

    if (row != -1)
        beginRow = row;

    else if (parent.isValid())
        beginRow = parent.row();

    QByteArray encodedData = data->data(mMimeType);
    QDataStream stream(&encodedData, QIODevice::ReadOnly);

    while (!stream.atEnd())
    {

        QString value;
        QStringList values;
        QStringList masters;

        for (int i = 0; i < EsmFile::FileProperty_Master; ++i)
        {
            stream >> value;
            values << value;
        }

        stream >> masters;

        insertRows(beginRow, 1);

        QModelIndex idx = index(beginRow++, 0, QModelIndex());
        setData(idx, QStringList() << values << masters, Qt::EditRole);
    }

    return true;
}

bool EsxModel::ContentModel::canBeChecked(const EsmFile *file) const
{
    //element can be checked if all its dependencies are
    foreach (const QString &master, file->masters())
        if (!isChecked(master))
            return false;

    return true;
}

void EsxModel::ContentModel::addFile(EsmFile *file)
{
    beginInsertRows(QModelIndex(), mFiles.count(), mFiles.count());
        mFiles.append(file);
    endInsertRows();
}

void EsxModel::ContentModel::addFiles(const QString &path)
{
    QDir dir(path);
    QStringList filters;
    filters << "*.esp" << "*.esm" << "*.omwgame" << "*.omwaddon";
    dir.setNameFilters(filters);

    // Create a decoder for non-latin characters in esx metadata
    QTextDecoder *decoder = mCodec->makeDecoder();

    foreach (const QString &path, dir.entryList())
    {
        QFileInfo info(dir.absoluteFilePath(path));
        EsmFile *file = new EsmFile(path);

        try {
            ESM::ESMReader fileReader;
            ToUTF8::Utf8Encoder encoder(ToUTF8::calculateEncoding(QString(mCodec->name()).toStdString()));
            fileReader.setEncoder(&encoder);
            fileReader.open(dir.absoluteFilePath(path).toStdString());

            foreach (const ESM::Header::MasterData &item, fileReader.getMasters())
                file->addMaster(QString::fromStdString(item.name));

            file->setAuthor     (decoder->toUnicode(fileReader.getAuthor().c_str()));
            file->setDate       (info.lastModified());
            file->setVersion    (fileReader.getFVer());
            file->setPath       (info.absoluteFilePath());
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

bool EsxModel::ContentModel::isChecked(const QString& name) const
{
    if (mCheckStates.contains(name))
        return (mCheckStates[name] == Qt::Checked);

    return false;
}

void EsxModel::ContentModel::setCheckState(const QString &name, bool isChecked)
{
    if (name.isEmpty())
        return;

    Qt::CheckState state = Qt::Unchecked;

    if (isChecked)
        state = Qt::Checked;

    mCheckStates[name] = state;
}

EsxModel::ContentFileList EsxModel::ContentModel::checkedItems() const
{
    ContentFileList list;

    foreach (EsmFile *file, mFiles)
    {
        if (isChecked(file->fileName()))
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
