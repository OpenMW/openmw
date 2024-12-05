#include "esmfile.hpp"

ContentSelectorModel::EsmFile::EsmFile(const QString& fileName, ModelItem* parent)
    : ModelItem(parent)
    , mFileName(fileName)
{
}

void ContentSelectorModel::EsmFile::setFileName(const QString& fileName)
{
    mFileName = fileName;
}

void ContentSelectorModel::EsmFile::setAuthor(const QString& author)
{
    mAuthor = author;
}

void ContentSelectorModel::EsmFile::setDate(const QDateTime& modified)
{
    mModified = modified;
}

void ContentSelectorModel::EsmFile::setFormat(const QString& format)
{
    mVersion = format;
}

void ContentSelectorModel::EsmFile::setFilePath(const QString& path)
{
    mPath = path;
}

void ContentSelectorModel::EsmFile::setGameFiles(const QStringList& gamefiles)
{
    mGameFiles = gamefiles;
}

void ContentSelectorModel::EsmFile::setDescription(const QString& description)
{
    mDescription = description;
}

void ContentSelectorModel::EsmFile::setBuiltIn(bool builtIn)
{
    mBuiltIn = builtIn;
}

void ContentSelectorModel::EsmFile::setFromAnotherConfigFile(bool fromAnotherConfigFile)
{
    mFromAnotherConfigFile = fromAnotherConfigFile;
}

bool ContentSelectorModel::EsmFile::isGameFile() const
{
    return (mGameFiles.size() == 0)
        && (mFileName.endsWith(QLatin1String(".esm"), Qt::CaseInsensitive)
            || mFileName.endsWith(QLatin1String(".omwgame"), Qt::CaseInsensitive));
}

QVariant ContentSelectorModel::EsmFile::fileProperty(const FileProperty prop) const
{
    switch (prop)
    {
        case FileProperty_FileName:
            return mFileName;
            break;

        case FileProperty_Author:
            return mAuthor;
            break;

        case FileProperty_Format:
            return mVersion;
            break;

        case FileProperty_DateModified:
            return mModified.toString(Qt::ISODate);
            break;

        case FileProperty_FilePath:
            return mPath;
            break;

        case FileProperty_Description:
            return mDescription;
            break;

        case FileProperty_BuiltIn:
            return mBuiltIn;
            break;

        case FileProperty_FromAnotherConfigFile:
            return mFromAnotherConfigFile;
            break;

        case FileProperty_GameFile:
            return mGameFiles;
            break;

        default:
            break;
    }
    return QVariant();
}
void ContentSelectorModel::EsmFile::setFileProperty(const FileProperty prop, const QString& value)
{
    switch (prop)
    {
        case FileProperty_FileName:
            mFileName = value;
            break;

        case FileProperty_Author:
            mAuthor = value;
            break;

        case FileProperty_Format:
            mVersion = value;
            break;

        case FileProperty_DateModified:
            mModified = QDateTime::fromString(value, Qt::ISODate);
            break;

        case FileProperty_FilePath:
            mPath = value;
            break;

        case FileProperty_Description:
            mDescription = value;
            break;

        // todo: check these work
        case FileProperty_BuiltIn:
            mBuiltIn = value == "true";
            break;

        case FileProperty_FromAnotherConfigFile:
            mFromAnotherConfigFile = value == "true";
            break;

        case FileProperty_GameFile:
            mGameFiles << value;
            break;

        default:
            break;
    }
}
