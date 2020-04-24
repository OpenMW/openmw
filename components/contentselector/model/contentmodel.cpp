#include "contentmodel.hpp"
#include "esmfile.hpp"

#include <stdexcept>

#include <QDir>
#include <QTextCodec>
#include <QDebug>

#include <components/esm/esmreader.hpp>

ContentSelectorModel::ContentModel::ContentModel(QObject *parent, QIcon warningIcon) :
    QAbstractTableModel(parent),
    mWarningIcon(warningIcon),
    mMimeType ("application/omwcontent"),
    mMimeTypes (QStringList() << mMimeType),
    mColumnCount (1),
    mDropActions (Qt::MoveAction)
{
    setEncoding ("win1252");
    uncheckAll();
}

ContentSelectorModel::ContentModel::~ContentModel()
{
    qDeleteAll(mFiles);
    mFiles.clear();
}

void ContentSelectorModel::ContentModel::setEncoding(const QString &encoding)
{
    mEncoding = encoding;
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

    for (const EsmFile *file : mFiles)
    {
        if (name.compare(file->fileProperty (fp).toString(), Qt::CaseInsensitive) == 0)
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
        return Qt::ItemIsDropEnabled;

    const EsmFile *file = item(index.row());

    if (!file)
        return Qt::NoItemFlags;

    //game files can always be checked
    if (file->isGameFile())
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;

    Qt::ItemFlags returnFlags;

    // addon can be checked if its gamefile is
    // ... special case, addon with no dependency can be used with any gamefile.
    bool gamefileChecked = (file->gameFiles().count() == 0);
    for (const QString &fileName : file->gameFiles())
    {
        for (QListIterator<EsmFile *> dependencyIter(mFiles); dependencyIter.hasNext(); dependencyIter.next())
        {
            //compare filenames only.  Multiple instances
            //of the filename (with different paths) is not relevant here.
            bool depFound = (dependencyIter.peekNext()->fileName().compare(fileName, Qt::CaseInsensitive) == 0);

            if (!depFound)
                continue;

            if (!gamefileChecked)
            {
                if (isChecked (dependencyIter.peekNext()->filePath()))
                    gamefileChecked = (dependencyIter.peekNext()->isGameFile());
            }

            // force it to iterate all files in cases where the current
            // dependency is a game file to ensure that a later duplicate
            // game file is / is not checked.
            // (i.e., break only if it's not a gamefile or the game file has been checked previously)
            if (gamefileChecked || !(dependencyIter.peekNext()->isGameFile()))
                break;
        }
    }

    if (gamefileChecked)
    {
        returnFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsDragEnabled;
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
    case Qt::DecorationRole:
    {
        return isLoadOrderError(file) ? mWarningIcon : QVariant();
    }

    case Qt::EditRole:
    case Qt::DisplayRole:
    {
        if (column >=0 && column <=EsmFile::FileProperty_GameFile)
            return file->fileProperty(static_cast<EsmFile::FileProperty>(column));

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
    }

    case Qt::ToolTipRole:
    {
        if (column != 0)
            return QVariant();

        return toolTip(file);
    }

    case Qt::CheckStateRole:
    {
        if (file->isGameFile())
            return QVariant();

        return mCheckStates[file->filePath()];
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
    }
    return QVariant();
}

bool ContentSelectorModel::ContentModel::setData(const QModelIndex &index, const QVariant &value, int role)
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
                success = setCheckState(file->filePath(), value.toBool());
                emit dataChanged(index, index);
            }
        }
        break;

        case Qt::CheckStateRole:
        {
            int checkValue = value.toInt();
            bool setState = false;
            if ((checkValue==Qt::Checked) && !isChecked(file->filePath()))
            {
                setState = true;
                success = true;
            }
            else if ((checkValue == Qt::Checked) && isChecked (file->filePath()))
                setState = true;
            else if (checkValue == Qt::Unchecked)
                setState = true;

            if (setState)
            {
                setCheckState(file->filePath(), success);
                emit dataChanged(index, index);
                checkForLoadOrderErrors();
            }
            else
                return success;

            for (EsmFile *file2 : mFiles)
            {
                if (file2->gameFiles().contains(fileName, Qt::CaseInsensitive))
                {
                    QModelIndex idx = indexFromItem(file2);
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

    // at this point we know that drag and drop has finished.
    checkForLoadOrderErrors();
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

    for (const QModelIndex &index : indexes)
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

    for (const QString &path2 : dir.entryList())
    {
        QFileInfo info(dir.absoluteFilePath(path2));

        if (item(info.fileName()))
            continue;

        try {
            ESM::ESMReader fileReader;
            ToUTF8::Utf8Encoder encoder =
            ToUTF8::calculateEncoding(mEncoding.toStdString());
            fileReader.setEncoder(&encoder);
            fileReader.open(std::string(dir.absoluteFilePath(path2).toUtf8().constData()));

            EsmFile *file = new EsmFile(path2);
         
            for (std::vector<ESM::Header::MasterData>::const_iterator itemIter = fileReader.getGameFiles().begin();
                itemIter != fileReader.getGameFiles().end(); ++itemIter)
                file->addGameFile(QString::fromUtf8(itemIter->name.c_str()));

            file->setAuthor     (QString::fromUtf8(fileReader.getAuthor().c_str()));
            file->setDate       (info.lastModified());
            file->setFormat     (fileReader.getFormat());
            file->setFilePath       (info.absoluteFilePath());
            file->setDescription(QString::fromUtf8(fileReader.getDesc().c_str()));

            // HACK
            // Load order constraint of Bloodmoon.esm needing Tribunal.esm is missing
            // from the file supplied by Bethesda, so we have to add it ourselves
            if (file->fileName().compare("Bloodmoon.esm", Qt::CaseInsensitive) == 0)
            {
                file->addGameFile(QString::fromUtf8("Tribunal.esm"));
            }

            // Put the file in the table
            addFile(file);

        } catch(std::runtime_error &e) {
            // An error occurred while reading the .esp
            qWarning() << "Error reading addon file: " << e.what();
            continue;
        }

    }

    sortFiles();
}

void ContentSelectorModel::ContentModel::clearFiles()
{
    const int filesCount = mFiles.count();

    if (filesCount > 0) {
        beginRemoveRows(QModelIndex(), 0, filesCount - 1);
        mFiles.clear();
        endRemoveRows();
    }
}

QStringList ContentSelectorModel::ContentModel::gameFiles() const
{
    QStringList gameFiles;
    for (const ContentSelectorModel::EsmFile *file : mFiles)
    {
        if (file->isGameFile())
        {
            gameFiles.append(file->fileName());
        }
    }
    return gameFiles;
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
                if (gamefiles.contains(mFiles.at(j)->fileName(), Qt::CaseInsensitive)
                 || (!mFiles.at(i)->isGameFile() && gamefiles.isEmpty()
                 && mFiles.at(j)->fileName().compare("Morrowind.esm", Qt::CaseInsensitive) == 0)) // Hack: implicit dependency on Morrowind.esm for dependency-less files
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

bool ContentSelectorModel::ContentModel::isChecked(const QString& filepath) const
{
    if (mCheckStates.contains(filepath))
        return (mCheckStates[filepath] == Qt::Checked);

    return false;
}

bool ContentSelectorModel::ContentModel::isEnabled (QModelIndex index) const
{
    return (flags(index) & Qt::ItemIsEnabled);
}

bool ContentSelectorModel::ContentModel::isLoadOrderError(const EsmFile *file) const
{
    return mPluginsWithLoadOrderError.contains(file->filePath());
}

void ContentSelectorModel::ContentModel::setContentList(const QStringList &fileList)
{
    mPluginsWithLoadOrderError.clear();
    int previousPosition = -1;
    for (const QString &filepath : fileList)
    {
        if (setCheckState(filepath, true))
        {
            // as necessary, move plug-ins in visible list to match sequence of supplied filelist
            const EsmFile* file = item(filepath);
            int filePosition = indexFromItem(file).row();
            if (filePosition < previousPosition)
            {
                mFiles.move(filePosition, previousPosition);
                emit dataChanged(index(filePosition, 0, QModelIndex()), index(previousPosition, 0, QModelIndex()));
            }
            else
            {
                previousPosition = filePosition;
            }
        }
    }
    checkForLoadOrderErrors();
}

void ContentSelectorModel::ContentModel::checkForLoadOrderErrors()
{
    for (int row = 0; row < mFiles.count(); ++row)
    {
        EsmFile* file = item(row);
        bool isRowInError = checkForLoadOrderErrors(file, row).count() != 0;
        if (isRowInError)
        {
            mPluginsWithLoadOrderError.insert(file->filePath());
        }
        else
        {
            mPluginsWithLoadOrderError.remove(file->filePath());
        }
    }
}

QList<ContentSelectorModel::LoadOrderError> ContentSelectorModel::ContentModel::checkForLoadOrderErrors(const EsmFile *file, int row) const
{
    QList<LoadOrderError> errors = QList<LoadOrderError>();
    for (const QString &dependentfileName : file->gameFiles())
    {
        const EsmFile* dependentFile = item(dependentfileName);

        if (!dependentFile)
        {
            errors.append(LoadOrderError(LoadOrderError::ErrorCode_MissingDependency, dependentfileName));
        }
        else
        {
            if (!isChecked(dependentFile->filePath()))
            {
                errors.append(LoadOrderError(LoadOrderError::ErrorCode_InactiveDependency, dependentfileName));
            }
            if (row < indexFromItem(dependentFile).row())
            {
                errors.append(LoadOrderError(LoadOrderError::ErrorCode_LoadOrder, dependentfileName));
            }
        }
    }
    return errors;
}

QString ContentSelectorModel::ContentModel::toolTip(const EsmFile *file) const
{
    if (isLoadOrderError(file))
    {
        QString text("<b>");
        int index = indexFromItem(item(file->filePath())).row();
        for (const LoadOrderError& error : checkForLoadOrderErrors(file, index))
        {
            text += "<p>";
            text += error.toolTip();
            text += "</p>";
        }
        text += ("</b>");
        text += file->toolTip();
        return text;
    }
    else
    {
        return file->toolTip();
    }
}

void ContentSelectorModel::ContentModel::refreshModel()
{
    emit dataChanged (index(0,0), index(rowCount()-1,0));
}

bool ContentSelectorModel::ContentModel::setCheckState(const QString &filepath, bool checkState)
{
    if (filepath.isEmpty())
        return false;

    const EsmFile *file = item(filepath);

    if (!file)
        return false;

    Qt::CheckState state = Qt::Unchecked;

    if (checkState)
        state = Qt::Checked;

    mCheckStates[filepath] = state;
    emit dataChanged(indexFromItem(item(filepath)), indexFromItem(item(filepath)));

    if (file->isGameFile())
        refreshModel();

    //if we're checking an item, ensure all "upstream" files (dependencies) are checked as well.
    if (state == Qt::Checked)
    {
        for (const QString& upstreamName : file->gameFiles())
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
        for (const EsmFile *downstreamFile : mFiles)
        {
            QFileInfo fileInfo(filepath);
            QString filename = fileInfo.fileName();

            if (downstreamFile->gameFiles().contains(filename, Qt::CaseInsensitive))
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
    for (EsmFile *file : mFiles)
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
