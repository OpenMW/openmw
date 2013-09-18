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

int EsxModel::ContentModel::rowCount(const QModelIndex &parent) const
{
    if(parent.isValid())
        return 0;

    return mFiles.size();
}

EsxModel::EsmFile* EsxModel::ContentModel::item(int row) const
{
    if (row >= 0 && row < mFiles.count())
        return mFiles.at(row);

    return 0;
}

EsxModel::EsmFile* EsxModel::ContentModel::findItem(const QString &name)
{
    for (int i = 0; i < mFiles.size(); ++i)
    {
        if (name == item(i)->fileName())
            return item(i);
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

    case Qt::CheckStateRole:
        if (!file->isMaster())
            return isChecked(file->fileName());
        break;

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

    switch(role)
    {
        case Qt::EditRole:
            {
                QStringList list = value.toStringList();

                //iterate the string list, assigning values to proeprties
                //index-enum correspondence 1:1
                for (int i = 0; i < EsxModel::Property_Master; i++)
                    file->setProperty(static_cast<EsmFileProperty>(i), list.at(i));

                //iterate the remainder of the string list, assifning everything
                // as
                for (int i = EsxModel::Property_Master; i < list.size(); i++)
                    file->setProperty (EsxModel::Property_Master, list.at(i));

                //emit data changed for the item itself
                emit dataChanged(index, index);

                return true;
            }
            break;

        case Qt::UserRole+1:
            {
                setCheckState(fileName, value.toBool());

                emit dataChanged(index, index);

                for(int i = 0; i < mFiles.size(); i++)
                {

                    if (mFiles.at(i)->masters().contains(fileName))
                    {
                        QModelIndex idx = QAbstractTableModel::index(i, 0);
                        emit dataChanged(idx, idx);
                    }
                }

                return true;
            }
            break;

        case Qt::CheckStateRole:
            {
                bool checked = ((value.toInt() == Qt::Checked) && !isChecked(fileName));

                setCheckState(fileName, checked);

                emit dataChanged(index, index);

                return true;
            }
            break;
    }

    return false;
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
    QByteArray encodedData;

    foreach (const QModelIndex &index, indexes)
    {
        if (!index.isValid())
            continue;

        QByteArray fileData = item(index.row())->encodedData();

        foreach (const char c, fileData)
            encodedData.append(c);
    }

    QMimeData *mimeData = new QMimeData();
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

    while (!stream.atEnd())
    {
        QStringList values;

        for (int i = 0; i < EsmFile::sPropertyCount; ++i)
            stream >> values;

        insertRows(beginRow, 1);

        QModelIndex idx = index(beginRow++, 0, QModelIndex());
        setData(idx, values, Qt::EditRole);
    }

    return true;
}

bool EsxModel::ContentModel::canBeChecked(const EsmFile *file) const
{
    //element can be checked if all its dependencies are
    foreach (const QString &master, file->masters())
    {
        {// if the master is not found in checkstates
         // or it is not specifically checked, return false
            if (!mCheckStates.contains(master))
                return false;

            if (!isChecked(master))
                return false;
        }

        bool found = false;

        //iterate each file, if it is not a master and
        //does not have a master that is currently checked,
        //return false.
        foreach(const EsmFile *file, mFiles)
        {
            QString filename = file->fileName();

            found = (filename == master);

            if (found)
            {
                if (!isChecked(filename))
                    return false;
            }
        }

        if (!found)
            return false;
    }

    return true;
}

void EsxModel::ContentModel::addFile(EsmFile *file)
{
    beginInsertRows(QModelIndex(), mFiles.count(), mFiles.count());
    {
        mFiles.append(file);
    } endInsertRows();
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
            //file->setSize(info.size());
            file->setDate(info.lastModified());
            file->setVersion(0.0f);
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

bool EsxModel::ContentModel::isChecked(const QString& name) const
{
    return (mCheckStates[name] == Qt::Checked);
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
