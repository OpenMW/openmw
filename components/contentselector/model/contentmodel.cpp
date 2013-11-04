#include "contentmodel.hpp"
#include "esmfile.hpp"

#include <QDir>
#include <QTextCodec>
#include <QDebug>

#include "components/esm/esmreader.hpp"

ContentSelectorModel::ContentModel::ContentModel(QObject *parent) :
    QAbstractTableModel(parent),
    mMimeType ("application/omwcontent"),
    mMimeTypes (QStringList() << mMimeType),
    mColumnCount (1),
    mDragDropFlags (Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled),
    mDropActions (Qt::CopyAction | Qt::MoveAction)
{
    setEncoding ("win1252");
    uncheckAll();
}

void ContentSelectorModel::ContentModel::setEncoding(const QString &encoding)
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

int ContentSelectorModel::ContentModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return mColumnCount;
}

int ContentSelectorModel::ContentModel::rowCount(const QModelIndex &parent) const
{
    if(parent.isValid())
        return 0;

    return mFiles.size();
}

const ContentSelectorModel::EsmFile *ContentSelectorModel::ContentModel::item(int row) const
{
    if (row >= 0 && row < mFiles.size())
        return mFiles.at(row);

    return 0;
}

ContentSelectorModel::EsmFile *ContentSelectorModel::ContentModel::item(int row)
{
    if (row >= 0 && row < mFiles.count())
        return mFiles.at(row);

    return 0;
}
const ContentSelectorModel::EsmFile *ContentSelectorModel::ContentModel::item(const QString &name) const
{
    EsmFile::FileProperty fp = EsmFile::FileProperty_FileName;

    if (name.contains ('/'))
        fp = EsmFile::FileProperty_FilePath;

    foreach (const EsmFile *file, mFiles)
    {
        if (name == file->fileProperty (fp).toString())
            return file;
    }
    return 0;
}

QModelIndex ContentSelectorModel::ContentModel::indexFromItem(const EsmFile *item) const
{
    //workaround: non-const pointer cast for calls from outside contentmodel/contentselector
    EsmFile *non_const_file_ptr = const_cast<EsmFile *>(item);

    if (item)
        return index(mFiles.indexOf(non_const_file_ptr),0);

    return QModelIndex();
}

Qt::ItemFlags ContentSelectorModel::ContentModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    const EsmFile *file = item(index.row());

    if (!file)
        return Qt::NoItemFlags;

    //game files can always be checked
    if (file->isGameFile())
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    Qt::ItemFlags returnFlags;
    bool allDependenciesFound = true;
    bool gamefileChecked = false;

    //addon can be checked if its gamefile is and all other dependencies exist
    foreach (const QString &fileName, file->gameFiles())
    {
        bool depFound = false;
        foreach (EsmFile *dependency, mFiles)
        {
            //compare filenames only.  Multiple instances
            //of the filename (with different paths) is not relevant here.
            depFound = (dependency->fileName() == fileName);

            if (!depFound)
                continue;

            if (!gamefileChecked)
            {
                if (isChecked (dependency->filePath()))
                    gamefileChecked = (dependency->isGameFile());
            }

            // force it to iterate all files in cases where the current
            // dependency is a game file to ensure that a later duplicate
            // game file is / is not checked.
            // (i.e., break only if it's not a gamefile or the game file has been checked previously)
            if (gamefileChecked || !(dependency->isGameFile()))
                break;
        }

        allDependenciesFound = allDependenciesFound && depFound;
    }

    if (gamefileChecked)
    {
        if (allDependenciesFound)
            returnFlags = returnFlags | Qt::ItemIsEnabled | Qt::ItemIsSelectable | mDragDropFlags;
        else
            returnFlags = Qt::ItemIsSelectable;
    }

    return returnFlags;
}

QVariant ContentSelectorModel::ContentModel::data(const QModelIndex &index, int role) const
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
        if (column >=0 && column <=EsmFile::FileProperty_GameFile)
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
        if (!file->isGameFile())
            return isChecked(file->filePath());
        break;
    }

    case Qt::UserRole:
    {
        if (file->isGameFile())
            return ContentType_GameFile;
        else
            if (flags(index))
                return ContentType_Addon;

        break;
    }

    case Qt::UserRole + 1:
        return isChecked(file->filePath());
        break;
    }
    return QVariant();
}

bool ContentSelectorModel::ContentModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!index.isValid())
        return false;

    EsmFile *file = item(index.row());
    QString fileName = file->filePath();
    bool success = false;

    switch(role)
    {
        case Qt::EditRole:
        {
            QStringList list = value.toStringList();

            for (int i = 0; i < EsmFile::FileProperty_GameFile; i++)
                file->setFileProperty(static_cast<EsmFile::FileProperty>(i), list.at(i));

            for (int i = EsmFile::FileProperty_GameFile; i < list.size(); i++)
                file->setFileProperty (EsmFile::FileProperty_GameFile, list.at(i));

            emit dataChanged(index, index);

            success = true;
        }
        break;

        case Qt::UserRole+1:
        {
            success = (flags (index) & Qt::ItemIsEnabled);

            if (success)
            {
                success = setCheckState(fileName, value.toBool());
                emit dataChanged(index, index);
            }
        }
        break;

        case Qt::CheckStateRole:
        {
            int checkValue = value.toInt();
            bool success = false;
            bool setState = false;
            if ((checkValue==Qt::Checked) && !isChecked(fileName))
            {
                setState = true;
                success = true;
            }
            else if ((checkValue == Qt::Checked) && isChecked (fileName))
                setState = true;
            else if (checkValue == Qt::Unchecked)
                setState = true;

            if (setState)
            {
                setCheckState(fileName, success);
                emit dataChanged(index, index);

            }
            else
                return success;


            foreach (EsmFile *file, mFiles)
            {
                if (file->gameFiles().contains(fileName))
                {
                    QModelIndex idx = indexFromItem(file);
                    emit dataChanged(idx, idx);
                }
            }

            success =  true;
        }
        break;
    }

    return success;
}

bool ContentSelectorModel::ContentModel::insertRows(int position, int rows, const QModelIndex &parent)
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

bool ContentSelectorModel::ContentModel::removeRows(int position, int rows, const QModelIndex &parent)
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

Qt::DropActions ContentSelectorModel::ContentModel::supportedDropActions() const
{
    return mDropActions;
}

QStringList ContentSelectorModel::ContentModel::mimeTypes() const
{
    return mMimeTypes;
}

QMimeData *ContentSelectorModel::ContentModel::mimeData(const QModelIndexList &indexes) const
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

bool ContentSelectorModel::ContentModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
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
        QStringList gamefiles;

        for (int i = 0; i < EsmFile::FileProperty_GameFile; ++i)
        {
            stream >> value;
            values << value;
        }

        stream >> gamefiles;

        insertRows(beginRow, 1);

        QModelIndex idx = index(beginRow++, 0, QModelIndex());
        setData(idx, QStringList() << values << gamefiles, Qt::EditRole);
    }

    return true;
}

void ContentSelectorModel::ContentModel::addFile(EsmFile *file)
{
    beginInsertRows(QModelIndex(), mFiles.count(), mFiles.count());
        mFiles.append(file);
    endInsertRows();

    QModelIndex idx = index (mFiles.size() - 2, 0, QModelIndex());

    emit dataChanged (idx, idx);
}

void ContentSelectorModel::ContentModel::addFiles(const QString &path)
{
    QDir dir(path);
    QStringList filters;
    filters << "*.esp" << "*.esm" << "*.omwgame" << "*.omwaddon";
    dir.setNameFilters(filters);

    QTextCodec *codec = QTextCodec::codecForName("UTF8");

    // Create a decoder for non-latin characters in esx metadata
    QTextDecoder *decoder = codec->makeDecoder();

    foreach (const QString &path, dir.entryList())
    {
        QFileInfo info(dir.absoluteFilePath(path));
        EsmFile *file = new EsmFile(path);

        try {
            ESM::ESMReader fileReader;
            ToUTF8::Utf8Encoder encoder =
            ToUTF8::calculateEncoding(QString(mCodec->name()).toStdString());
            fileReader.setEncoder(&encoder);
            fileReader.open(dir.absoluteFilePath(path).toStdString());

            foreach (const ESM::Header::MasterData &item, fileReader.getGameFiles())
                file->addGameFile(QString::fromStdString(item.name));

            file->setAuthor     (decoder->toUnicode(fileReader.getAuthor().c_str()));
            file->setDate       (info.lastModified());
            file->setFormat     (fileReader.getFormat());
            file->setFilePath       (info.absoluteFilePath());
            file->setDescription(decoder->toUnicode(fileReader.getDesc().c_str()));


            // Put the file in the table
            if (item(file->filePath()) == 0)
                addFile(file);

        } catch(std::runtime_error &e) {
            // An error occurred while reading the .esp
            qWarning() << "Error reading addon file: " << e.what();
            continue;
        }

    }

    delete decoder;

    sortFiles();
}

void ContentSelectorModel::ContentModel::sortFiles()
{
    //first, sort the model such that all dependencies are ordered upstream (gamefile) first.
    bool movedFiles = true;
    int fileCount = mFiles.size();

    //Dependency sort
    //iterate until no sorting of files occurs
    while (movedFiles)
    {
        movedFiles = false;
        //iterate each file, obtaining a reference to it's gamefiles list
        for (int i = 0; i < fileCount; i++)
        {
            QModelIndex idx1 = index (i, 0, QModelIndex());
            const QStringList &gamefiles = mFiles.at(i)->gameFiles();
            //iterate each file after the current file, verifying that none of it's
            //dependencies appear.
            for (int j = i + 1; j < fileCount; j++)
            {
                if (gamefiles.contains(mFiles.at(j)->fileName()))
                {
                        mFiles.move(j, i);

                        QModelIndex idx2 = index (j, 0, QModelIndex());

                        emit dataChanged (idx1, idx2);

                        movedFiles = true;
                }
            }
            if (movedFiles)
                break;
        }
    }
}

bool ContentSelectorModel::ContentModel::isChecked(const QString& name) const
{
    if (mCheckStates.contains(name))
        return (mCheckStates[name] == Qt::Checked);

    return false;
}

bool ContentSelectorModel::ContentModel::isEnabled (QModelIndex index) const
{
    return (flags(index) & Qt::ItemIsEnabled);
}

void ContentSelectorModel::ContentModel::setCheckStates (const QStringList &fileList, bool isChecked)
{
    foreach (const QString &file, fileList)
    {
        setCheckState (file, isChecked);
    }
}

void ContentSelectorModel::ContentModel::refreshModel()
{
    emit dataChanged (index(0,0), index(rowCount()-1,0));
}

bool ContentSelectorModel::ContentModel::setCheckState(const QString &name, bool checkState)
{
    if (name.isEmpty())
        return false;

    const EsmFile *file = item(name);

    if (!file)
        return false;

    Qt::CheckState state = Qt::Unchecked;

    if (checkState)
        state = Qt::Checked;

    mCheckStates[name] = state;
    emit dataChanged(indexFromItem(item(name)), indexFromItem(item(name)));

    if (file->isGameFile())
        refreshModel();

    //if we're checking an item, ensure all "upstream" files (dependencies) are checked as well.
    if (state == Qt::Checked)
    {
        foreach (QString upstreamName, file->gameFiles())
        {
            const EsmFile *upstreamFile = item(upstreamName);

            if (!upstreamFile)
                continue;

            if (!isChecked(upstreamFile->filePath()))
                mCheckStates[upstreamFile->filePath()] = Qt::Checked;

            emit dataChanged(indexFromItem(upstreamFile), indexFromItem(upstreamFile));

        }
    }
    //otherwise, if we're unchecking an item (or the file is a game file) ensure all downstream files are unchecked.
    if (state == Qt::Unchecked)
    {
        foreach (const EsmFile *downstreamFile, mFiles)
        {
            if (downstreamFile->gameFiles().contains(name))
            {
                if (mCheckStates.contains(downstreamFile->filePath()))
                    mCheckStates[downstreamFile->filePath()] = Qt::Unchecked;

                emit dataChanged(indexFromItem(downstreamFile), indexFromItem(downstreamFile));
            }
        }
    }

    return true;
}

ContentSelectorModel::ContentFileList ContentSelectorModel::ContentModel::checkedItems() const
{
    ContentFileList list;

    // TODO:
    // First search for game files and next addons,
    // so we get more or less correct game files vs addons order.
    foreach (EsmFile *file, mFiles)
        if (isChecked(file->filePath()))
            list << file;

    return list;
}

void ContentSelectorModel::ContentModel::uncheckAll()
{
    emit layoutAboutToBeChanged();
    mCheckStates.clear();
    emit layoutChanged();
}
