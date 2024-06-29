#include "contentmodel.hpp"
#include "esmfile.hpp"

#include <algorithm>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <unordered_set>

#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFont>
#include <QIODevice>

#include <components/esm/format.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm4/reader.hpp>
#include <components/files/openfile.hpp>
#include <components/files/qtconversion.hpp>

ContentSelectorModel::ContentModel::ContentModel(QObject* parent, QIcon& warningIcon, bool showOMWScripts)
    : QAbstractTableModel(parent)
    , mWarningIcon(warningIcon)
    , mShowOMWScripts(showOMWScripts)
    , mMimeType("application/omwcontent")
    , mMimeTypes(QStringList() << mMimeType)
    , mColumnCount(1)
    , mDropActions(Qt::MoveAction)
{
    setEncoding("win1252");
    uncheckAll();
}

ContentSelectorModel::ContentModel::~ContentModel()
{
    qDeleteAll(mFiles);
    mFiles.clear();
}

void ContentSelectorModel::ContentModel::setEncoding(const QString& encoding)
{
    mEncoding = encoding;
}

int ContentSelectorModel::ContentModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return mColumnCount;
}

int ContentSelectorModel::ContentModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return mFiles.size();
}

const ContentSelectorModel::EsmFile* ContentSelectorModel::ContentModel::item(int row) const
{
    if (row >= 0 && row < mFiles.size())
        return mFiles.at(row);

    return nullptr;
}

ContentSelectorModel::EsmFile* ContentSelectorModel::ContentModel::item(int row)
{
    if (row >= 0 && row < mFiles.count())
        return mFiles.at(row);

    return nullptr;
}
const ContentSelectorModel::EsmFile* ContentSelectorModel::ContentModel::item(const QString& name) const
{
    EsmFile::FileProperty fp = EsmFile::FileProperty_FileName;

    if (name.contains('/'))
        fp = EsmFile::FileProperty_FilePath;

    for (const EsmFile* file : mFiles)
    {
        if (name.compare(file->fileProperty(fp).toString(), Qt::CaseInsensitive) == 0)
            return file;
    }
    return nullptr;
}

QModelIndex ContentSelectorModel::ContentModel::indexFromItem(const EsmFile* item) const
{
    // workaround: non-const pointer cast for calls from outside contentmodel/contentselector
    EsmFile* non_const_file_ptr = const_cast<EsmFile*>(item);

    if (item)
        return index(mFiles.indexOf(non_const_file_ptr), 0);

    return QModelIndex();
}

Qt::ItemFlags ContentSelectorModel::ContentModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::ItemIsDropEnabled;

    const EsmFile* file = item(index.row());

    if (!file)
        return Qt::NoItemFlags;

    if (file->builtIn() || file->fromAnotherConfigFile())
        return Qt::ItemIsEnabled;

    // game files can always be checked
    if (file == mGameFile)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;

    Qt::ItemFlags returnFlags;

    // addon can be checked if its gamefile is
    // ... special case, addon with no dependency can be used with any gamefile.
    bool gamefileChecked = false;
    bool noGameFiles = true;
    for (const QString& fileName : file->gameFiles())
    {
        for (QListIterator<EsmFile*> dependencyIter(mFiles); dependencyIter.hasNext(); dependencyIter.next())
        {
            // compare filenames only.  Multiple instances
            // of the filename (with different paths) is not relevant here.
            EsmFile* depFile = dependencyIter.peekNext();
            if (!depFile->isGameFile() || depFile->fileName().compare(fileName, Qt::CaseInsensitive) != 0)
                continue;

            noGameFiles = false;
            if (depFile->builtIn() || depFile->fromAnotherConfigFile() || mCheckedFiles.contains(depFile))
            {
                gamefileChecked = true;
                break;
            }
        }
    }

    if (gamefileChecked || noGameFiles)
    {
        returnFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsDragEnabled;
    }

    return returnFlags;
}

QVariant ContentSelectorModel::ContentModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= mFiles.size())
        return QVariant();

    const EsmFile* file = item(index.row());

    if (!file)
        return QVariant();

    const int column = index.column();

    switch (role)
    {
        case Qt::DecorationRole:
        {
            return isLoadOrderError(file) ? mWarningIcon : QVariant();
        }

        case Qt::FontRole:
        {
            if (isNew(file->fileName()))
            {
                auto font = QFont();
                font.setBold(true);
                font.setItalic(true);
                return font;
            }
            return QVariant();
        }

        case Qt::EditRole:
        case Qt::DisplayRole:
        {
            if (column >= 0 && column <= EsmFile::FileProperty_GameFile)
                return file->fileProperty(static_cast<EsmFile::FileProperty>(column));

            return QVariant();
        }

        case Qt::TextAlignmentRole:
        {
            switch (column)
            {
                case 0:
                case 1:
                    return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
                case 2:
                case 3:
                    return QVariant(Qt::AlignRight | Qt::AlignVCenter);
                default:
                    return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
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
            if (file == mGameFile)
                return QVariant();

            return (file->builtIn() || file->fromAnotherConfigFile() || mCheckedFiles.contains(file)) ? Qt::Checked
                                                                                                      : Qt::Unchecked;
        }

        case Qt::UserRole:
        {
            if (file == mGameFile)
                return ContentType_GameFile;
            else if (flags(index))
                return ContentType_Addon;

            break;
        }

        case Qt::UserRole + 1:
            return mCheckedFiles.contains(file);
    }
    return QVariant();
}

bool ContentSelectorModel::ContentModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid())
        return false;

    EsmFile* file = item(index.row());
    QString fileName = file->fileName();
    bool success = false;

    switch (role)
    {
        case Qt::EditRole:
        {
            QStringList list = value.toStringList();

            for (int i = 0; i < EsmFile::FileProperty_GameFile; i++)
                file->setFileProperty(static_cast<EsmFile::FileProperty>(i), list.at(i));

            for (int i = EsmFile::FileProperty_GameFile; i < list.size(); i++)
                file->setFileProperty(EsmFile::FileProperty_GameFile, list.at(i));

            emit dataChanged(index, index);

            success = true;
        }
        break;

        case Qt::UserRole + 1:
        {
            success = (flags(index) & Qt::ItemIsEnabled);

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
            if (file->builtIn() || file->fromAnotherConfigFile())
            {
                setState = false;
                success = false;
            }
            else if (checkValue == Qt::Checked && !mCheckedFiles.contains(file))
            {
                setState = true;
                success = true;
            }
            else if (checkValue == Qt::Checked && mCheckedFiles.contains(file))
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

            for (EsmFile* file2 : mFiles)
            {
                if (file2->gameFiles().contains(fileName, Qt::CaseInsensitive))
                {
                    QModelIndex idx = indexFromItem(file2);
                    emit dataChanged(idx, idx);
                }
            }

            success = true;
        }
        break;
    }

    return success;
}

bool ContentSelectorModel::ContentModel::insertRows(int position, int rows, const QModelIndex& parent)
{
    return false;
}

bool ContentSelectorModel::ContentModel::removeRows(int position, int rows, const QModelIndex& parent)
{
    return false;
}

Qt::DropActions ContentSelectorModel::ContentModel::supportedDropActions() const
{
    return mDropActions;
}

QStringList ContentSelectorModel::ContentModel::mimeTypes() const
{
    return mMimeTypes;
}

QMimeData* ContentSelectorModel::ContentModel::mimeData(const QModelIndexList& indexes) const
{
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    for (const QModelIndex& index : indexes)
    {
        if (!index.isValid())
            continue;

        stream << index.row();
    }

    QMimeData* mimeData = new QMimeData();
    mimeData->setData(mMimeType, encodedData);

    return mimeData;
}

bool ContentSelectorModel::ContentModel::dropMimeData(
    const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
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

    int firstModifiable = 0;
    while (item(firstModifiable)->builtIn() || item(firstModifiable)->fromAnotherConfigFile())
        ++firstModifiable;

    if (beginRow < firstModifiable)
        return false;

    QByteArray encodedData = data->data(mMimeType);
    QDataStream stream(&encodedData, QIODevice::ReadOnly);

    std::vector<EsmFile*> toMove;
    while (!stream.atEnd())
    {
        int sourceRow;
        stream >> sourceRow;
        toMove.emplace_back(mFiles.at(sourceRow));
    }
    int minRow = mFiles.size();
    int maxRow = 0;
    for (EsmFile* file : toMove)
    {
        int from = mFiles.indexOf(file);
        int to = beginRow;
        if (from < beginRow)
            to--;
        else if (from > beginRow)
            beginRow++;
        minRow = std::min(minRow, std::min(to, from));
        maxRow = std::max(maxRow, std::max(to, from));
        mFiles.move(from, to);
    }

    dataChanged(index(minRow, 0), index(maxRow, 0));
    // at this point we know that drag and drop has finished.
    checkForLoadOrderErrors();

    return true;
}

void ContentSelectorModel::ContentModel::addFile(EsmFile* file)
{
    beginInsertRows(QModelIndex(), mFiles.count(), mFiles.count());
    mFiles.append(file);
    endInsertRows();

    QModelIndex idx = index(mFiles.size() - 2, 0, QModelIndex());

    emit dataChanged(idx, idx);
}

void ContentSelectorModel::ContentModel::addFiles(const QString& path, bool newfiles)
{
    QDir dir(path);
    QStringList filters;
    filters << "*.esp"
            << "*.esm"
            << "*.omwgame"
            << "*.omwaddon";
    if (mShowOMWScripts)
        filters << "*.omwscripts";
    dir.setNameFilters(filters);
    dir.setSorting(QDir::Name);

    for (const QString& path2 : dir.entryList())
    {
        QFileInfo info(dir.absoluteFilePath(path2));

        EsmFile* file = const_cast<EsmFile*>(item(info.fileName()));
        bool add = file == nullptr;
        std::unique_ptr<EsmFile> newFile;
        if (add)
        {
            newFile = std::make_unique<EsmFile>(path2);
            file = newFile.get();
        }
        else
        {
            // We've found the same file in a higher priority dir, update our existing entry
            file->setFileName(path2);
            file->setGameFiles({});
        }

        if (info.fileName().compare("builtin.omwscripts", Qt::CaseInsensitive) == 0)
            file->setBuiltIn(true);

        file->setFromAnotherConfigFile(mNonUserContent.contains(info.fileName().toLower()));

        if (info.fileName().endsWith(".omwscripts", Qt::CaseInsensitive))
        {
            file->setDate(info.lastModified());
            file->setFilePath(info.absoluteFilePath());
            if (add)
                addFile(newFile.release());
            setNew(file->fileName(), newfiles);
            continue;
        }

        try
        {
            file->setDate(info.lastModified());
            file->setFilePath(info.absoluteFilePath());
            std::filesystem::path filepath = Files::pathFromQString(info.absoluteFilePath());

            auto stream = Files::openBinaryInputFileStream(filepath);
            if (!stream->is_open())
            {
                qWarning() << "Failed to open addon file " << info.fileName() << ": "
                           << std::generic_category().message(errno).c_str();
                continue;
            }
            const ESM::Format format = ESM::readFormat(*stream);
            stream->seekg(0);
            switch (format)
            {
                case ESM::Format::Tes3:
                {
                    ToUTF8::Utf8Encoder encoder(ToUTF8::calculateEncoding(mEncoding.toStdString()));
                    ESM::ESMReader fileReader;
                    fileReader.setEncoder(&encoder);
                    fileReader.open(std::move(stream), filepath);
                    file->setAuthor(QString::fromUtf8(fileReader.getAuthor().c_str()));
                    file->setFormat(QString::number(fileReader.esmVersionF()));
                    file->setDescription(QString::fromUtf8(fileReader.getDesc().c_str()));
                    for (const auto& master : fileReader.getGameFiles())
                        file->addGameFile(QString::fromUtf8(master.name.c_str()));
                    break;
                }
                case ESM::Format::Tes4:
                {
                    ToUTF8::StatelessUtf8Encoder encoder(ToUTF8::calculateEncoding(mEncoding.toStdString()));
                    ESM4::Reader fileReader(std::move(stream), filepath, nullptr, &encoder, true);
                    file->setAuthor(QString::fromUtf8(fileReader.getAuthor().c_str()));
                    file->setFormat(QString::number(fileReader.esmVersionF()));
                    file->setDescription(QString::fromUtf8(fileReader.getDesc().c_str()));
                    for (const auto& master : fileReader.getGameFiles())
                        file->addGameFile(QString::fromUtf8(master.name.c_str()));
                    break;
                }
                default:
                {
                    qWarning() << "Error reading addon file " << info.fileName() << ": unsupported ESM format "
                               << ESM::NAME(format).toString().c_str();
                    continue;
                }
            }

            // Put the file in the table
            if (add)
                addFile(newFile.release());
            setNew(file->fileName(), newfiles);
        }
        catch (std::runtime_error& e)
        {
            // An error occurred while reading the .esp
            qWarning() << "Error reading addon file: " << e.what();
        }
    }
}

bool ContentSelectorModel::ContentModel::containsDataFiles(const QString& path)
{
    QDir dir(path);
    QStringList filters;
    filters << "*.esp"
            << "*.esm"
            << "*.omwgame"
            << "*.omwaddon";
    dir.setNameFilters(filters);

    return dir.entryList().count() != 0;
}

void ContentSelectorModel::ContentModel::clearFiles()
{
    const int filesCount = mFiles.count();

    if (filesCount > 0)
    {
        beginRemoveRows(QModelIndex(), 0, filesCount - 1);
        qDeleteAll(mFiles);
        mFiles.clear();
        endRemoveRows();
    }
}

QStringList ContentSelectorModel::ContentModel::gameFiles() const
{
    QStringList gameFiles;
    for (const ContentSelectorModel::EsmFile* file : mFiles)
    {
        if (file->isGameFile())
        {
            gameFiles.append(file->fileName());
        }
    }
    return gameFiles;
}

void ContentSelectorModel::ContentModel::setCurrentGameFile(const EsmFile* file)
{
    QModelIndex oldIndex = indexFromItem(mGameFile);
    QModelIndex index = indexFromItem(file);
    mGameFile = file;
    emit dataChanged(oldIndex, oldIndex);
    emit dataChanged(index, index);
}

void ContentSelectorModel::ContentModel::sortFiles()
{
    emit layoutAboutToBeChanged();

    int firstModifiable = 0;
    while (firstModifiable < mFiles.size()
        && (mFiles.at(firstModifiable)->builtIn() || mFiles.at(firstModifiable)->fromAnotherConfigFile()))
        ++firstModifiable;

    // For the purposes of dependency sort we'll hallucinate that Bloodmoon is dependent on Tribunal
    const EsmFile* tribunalFile = item("Tribunal.esm");
    const EsmFile* bloodmoonFile = item("Bloodmoon.esm");
    const bool sortExpansions = tribunalFile != nullptr && bloodmoonFile != nullptr;

    // Dependency sort
    std::unordered_set<const EsmFile*> moved;
    for (int i = mFiles.size() - 1; i > firstModifiable;)
    {
        const auto file = mFiles.at(i);
        if (moved.find(file) == moved.end())
        {
            int index = -1;
            for (int j = firstModifiable; j < i; ++j)
            {
                const EsmFile* addonFile = mFiles.at(j);
                const QStringList& gameFiles = addonFile->gameFiles();
                // All addon files are implicitly dependent on the game file
                // so that they don't accidentally become the game file
                if (gameFiles.contains(file->fileName(), Qt::CaseInsensitive) || file == mGameFile
                    || (sortExpansions && file == tribunalFile && addonFile == bloodmoonFile))
                {
                    index = j;
                    break;
                }
            }
            if (index >= 0)
            {
                mFiles.move(i, index);
                moved.insert(file);
                continue;
            }
        }
        --i;
        moved.clear();
    }
    emit layoutChanged();
}

bool ContentSelectorModel::ContentModel::isEnabled(const QModelIndex& index) const
{
    return (flags(index) & Qt::ItemIsEnabled);
}

bool ContentSelectorModel::ContentModel::isNew(const QString& filepath) const
{
    const auto it = mNewFiles.find(filepath);
    if (it == mNewFiles.end())
        return false;
    return it.value();
}

void ContentSelectorModel::ContentModel::setNew(const QString& filepath, bool isNew)
{
    if (filepath.isEmpty())
        return;

    const EsmFile* file = item(filepath);

    if (!file)
        return;

    mNewFiles[filepath] = isNew;
}

void ContentSelectorModel::ContentModel::setNonUserContent(const QStringList& fileList)
{
    mNonUserContent.clear();
    for (const auto& file : fileList)
        mNonUserContent.insert(file.toLower());
    for (auto* file : mFiles)
        file->setFromAnotherConfigFile(mNonUserContent.contains(file->fileName().toLower()));

    auto insertPosition
        = std::ranges::find_if(mFiles, [](const EsmFile* file) { return !file->builtIn(); }) - mFiles.begin();

    for (const auto& filepath : fileList)
    {
        const EsmFile* file = item(filepath);
        int filePosition = indexFromItem(file).row();
        mFiles.move(filePosition, insertPosition++);
    }

    sortFiles();
}

bool ContentSelectorModel::ContentModel::isLoadOrderError(const EsmFile* file) const
{
    return mPluginsWithLoadOrderError.contains(file->filePath());
}

void ContentSelectorModel::ContentModel::setContentList(const QStringList& fileList)
{
    mPluginsWithLoadOrderError.clear();
    int previousPosition = -1;
    for (const QString& filepath : fileList)
    {
        if (setCheckState(filepath, true))
        {
            // setCheckState already gracefully handles builtIn and fromAnotherConfigFile
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
        EsmFile* file = mFiles.at(row);
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

QList<ContentSelectorModel::LoadOrderError> ContentSelectorModel::ContentModel::checkForLoadOrderErrors(
    const EsmFile* file, int row) const
{
    QList<LoadOrderError> errors = QList<LoadOrderError>();
    for (const QString& dependentfileName : file->gameFiles())
    {
        const EsmFile* dependentFile = item(dependentfileName);

        if (!dependentFile)
        {
            errors.append(LoadOrderError(LoadOrderError::ErrorCode_MissingDependency, dependentfileName));
        }
        else
        {
            if (!mCheckedFiles.contains(dependentFile))
            {
                errors.append(LoadOrderError(LoadOrderError::ErrorCode_InactiveDependency, dependentfileName));
            }
            if (row < indexFromItem(dependentFile).row())
            {
                errors.append(LoadOrderError(LoadOrderError::ErrorCode_LoadOrder, dependentfileName));
            }
        }
    }

    if (file->fileName().compare("Bloodmoon.esm", Qt::CaseInsensitive) == 0)
    {
        // Warn the user if Bloodmoon is loaded before Tribunal (Tribunal is not a hard dependency)
        const EsmFile* tribunalFile = item("Tribunal.esm");
        if (tribunalFile != nullptr && mCheckedFiles.contains(tribunalFile) && row < indexFromItem(tribunalFile).row())
            errors.append(LoadOrderError(LoadOrderError::ErrorCode_LoadOrder, "Tribunal.esm"));
    }

    return errors;
}

QString ContentSelectorModel::ContentModel::toolTip(const EsmFile* file) const
{
    if (isLoadOrderError(file))
    {
        QString text("<b>");
        int index = indexFromItem(item(file->filePath())).row();
        for (const LoadOrderError& error : checkForLoadOrderErrors(file, index))
        {
            assert(error.errorCode() != LoadOrderError::ErrorCode::ErrorCode_None);

            text += "<p>";
            text += mErrorToolTips[error.errorCode() - 1].arg(error.fileName());
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
    emit dataChanged(index(0, 0), index(rowCount() - 1, 0));
}

bool ContentSelectorModel::ContentModel::setCheckState(const QString& filepath, bool checkState)
{
    if (filepath.isEmpty())
        return false;

    const EsmFile* file = item(filepath);

    if (!file || file->builtIn() || file->fromAnotherConfigFile())
        return false;

    if (checkState)
        mCheckedFiles.insert(file);
    else
        mCheckedFiles.erase(file);

    emit dataChanged(indexFromItem(item(filepath)), indexFromItem(item(filepath)));

    if (file->isGameFile())
        refreshModel();

    // if we're checking an item, ensure all "upstream" files (dependencies) are checked as well.
    if (checkState)
    {
        for (const QString& upstreamName : file->gameFiles())
        {
            const EsmFile* upstreamFile = item(upstreamName);

            if (!upstreamFile)
                continue;

            mCheckedFiles.insert(upstreamFile);

            emit dataChanged(indexFromItem(upstreamFile), indexFromItem(upstreamFile));
        }
    }
    // otherwise, if we're unchecking an item (or the file is a game file) ensure all downstream files are unchecked.
    else
    {
        for (const EsmFile* downstreamFile : mFiles)
        {
            QFileInfo fileInfo(filepath);
            QString filename = fileInfo.fileName();

            if (downstreamFile->gameFiles().contains(filename, Qt::CaseInsensitive))
            {
                mCheckedFiles.erase(downstreamFile);

                emit dataChanged(indexFromItem(downstreamFile), indexFromItem(downstreamFile));
            }
        }
    }

    // Need to manually let Bloodmoon entry know if Tribunal is checked/unchecked
    if (file->fileName().compare("Tribunal.esm", Qt::CaseInsensitive) == 0)
    {
        const EsmFile* bloodmoonFile = item("Bloodmoon.esm");
        if (bloodmoonFile != nullptr)
        {
            QModelIndex bloodmoonIndex = indexFromItem(bloodmoonFile);
            emit dataChanged(bloodmoonIndex, bloodmoonIndex);
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
    for (EsmFile* file : mFiles)
        if (mCheckedFiles.contains(file))
            list << file;

    return list;
}

void ContentSelectorModel::ContentModel::uncheckAll()
{
    emit layoutAboutToBeChanged();
    mCheckedFiles.clear();
    emit layoutChanged();
}
