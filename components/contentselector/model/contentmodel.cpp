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
#include <QDirIterator>
#include <QFont>
#include <QIODevice>

#include <components/esm/format.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm4/reader.hpp>
#include <components/files/openfile.hpp>
#include <components/files/qtconversion.hpp>

ContentSelectorModel::ContentModel::ContentModel(
    QObject* parent, QIcon& warningIcon, QIcon& errorIcon, bool showOMWScripts)
    : QAbstractTableModel(parent)
    , mWarningIcon(warningIcon)
    , mErrorIcon(errorIcon)
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
    bool path = name.contains('/');
    for (const EsmFile* file : mFiles)
    {
        if (name.compare(path ? file->filePath() : file->fileName(), Qt::CaseInsensitive) == 0)
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
            if (file->isMissing())
                return mErrorIcon;
            else if (isLoadOrderError(file))
                return mWarningIcon;
            else
                return QVariant();
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
            return true;
        }
        case Qt::UserRole + 1:
        {
            return isEnabled(index) && setCheckState(file, value.toBool());
        }
        case Qt::CheckStateRole:
        {
            int checkValue = value.toInt();
            if (checkValue == Qt::Checked)
                return mCheckedFiles.contains(file) || setCheckState(file, true);
            if (checkValue == Qt::Unchecked)
                return !mCheckedFiles.contains(file) || setCheckState(file, false);
        }
    }

    return false;
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
    QStringList filters;
    filters << "*.esp"
            << "*.esm"
            << "*.omwgame"
            << "*.omwaddon";
    QDirIterator it(path, filters, QDir::Files | QDir::NoDotAndDotDot);
    return it.hasNext();
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

    // make both Qt5 (int) and Qt6 (qsizetype aka size_t) happy
    using index_t = ContentFileList::size_type;

    // ensure built-in are first
    index_t firstModifiable = 0;
    for (index_t i = 0; i < mFiles.length(); ++i)
    {
        if (mFiles.at(i)->builtIn())
            mFiles.move(i, firstModifiable++);
    }

    // then non-user content
    for (const auto& filename : mNonUserContent)
    {
        const EsmFile* file = item(filename);
        int filePosition = indexFromItem(file).row();
        if (filePosition >= 0)
            mFiles.move(filePosition, firstModifiable++);
        else
        {
            // the file is not in the VFS, and will be displayed with an error
            auto missingFile = std::make_unique<EsmFile>(filename);
            missingFile->setFromAnotherConfigFile(true);
            mFiles.insert(firstModifiable++, missingFile.release());
        }
    }

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
        mNonUserContent.append(file.toLower());
    for (auto* file : mFiles)
        file->setFromAnotherConfigFile(mNonUserContent.contains(file->fileName().toLower()));

    sortFiles();
}

bool ContentSelectorModel::ContentModel::isLoadOrderError(const EsmFile* file) const
{
    int index = indexFromItem(file).row();
    auto errors = checkForLoadOrderErrors(file, index);
    return !errors.empty();
}

void ContentSelectorModel::ContentModel::setContentList(const QStringList& fileList)
{
    int previousPosition = -1;
    for (const QString& filepath : fileList)
    {
        const EsmFile* file = item(filepath);
        if (setCheckState(file, true))
        {
            // setCheckState already gracefully handles builtIn and fromAnotherConfigFile
            // as necessary, move plug-ins in visible list to match sequence of supplied filelist
            // FIXME: setCheckState also does tons of other things which we don't want to happen
            int filePosition = indexFromItem(file).row();
            if (filePosition < previousPosition)
            {
                mFiles.move(filePosition, previousPosition);
            }
            else
            {
                previousPosition = filePosition;
            }
        }
    }
    refreshModel();
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
    int index = indexFromItem(file).row();
    auto errors = checkForLoadOrderErrors(file, index);
    if (!errors.empty())
    {
        QString text("<b>");
        for (const LoadOrderError& error : errors)
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

void ContentSelectorModel::ContentModel::refreshModel(std::initializer_list<int> roles)
{
    emit dataChanged(index(0, 0), index(rowCount() - 1, 0), roles);
}

bool ContentSelectorModel::ContentModel::setCheckState(const EsmFile* file, bool checkState)
{
    if (!file || file->builtIn() || file->fromAnotherConfigFile())
        return false;

    if (checkState)
        mCheckedFiles.insert(file);
    else
        mCheckedFiles.erase(file);

    QModelIndex fileIndex = indexFromItem(file);
    emit dataChanged(fileIndex, fileIndex);

    // FIXME: this should not happen per-file.
    // Consider not hiding files if their game is disabled so that this is completely unnecessary.
    if (file->isGameFile())
        refreshModel();

    // Check "upstream" files (dependencies) if the file is checked,
    // uncheck downstream files if the file is unchecked.
    // Update the data for downstream files unconditionally (load order warnings).
    // FIXME: downstream files of toggled upstream/downstream files should be updated, but that would be slow.
    if (checkState)
    {
        for (const QString& upstreamName : file->gameFiles())
        {
            const EsmFile* upstreamFile = item(upstreamName);
            if (upstreamFile == nullptr || !mCheckedFiles.insert(upstreamFile).second)
                continue;
            QModelIndex upstreamIndex = indexFromItem(upstreamFile);
            emit dataChanged(upstreamIndex, upstreamIndex);
        }
    }
    for (const EsmFile* otherFile : mFiles)
    {
        if (!otherFile->gameFiles().contains(file->fileName(), Qt::CaseInsensitive))
            continue;
        if (!checkState)
            mCheckedFiles.erase(otherFile);
        QModelIndex otherIndex = indexFromItem(otherFile);
        emit dataChanged(otherIndex, otherIndex);
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
    mCheckedFiles.clear();
    refreshModel({ Qt::CheckStateRole, Qt::UserRole + 1 });
}
