#include "esmfile.hpp"

#include <QMimeData>
#include <QDataStream>

int ContentSelectorModel::EsmFile::sPropertyCount = 7;
QString ContentSelectorModel::EsmFile::sToolTip = QString("<b>Author:</b> %1<br/> \
                                              <b>Version:</b> %2<br/> \
                                              <b>Modified:</b> %3<br/> \
                                              <b>Path:</b><br/>%4<br/> \
                                              <br/><b>Description:</b><br/>%5<br/> \
                                              <br/><b>Dependencies: </b>%6<br/>");


ContentSelectorModel::EsmFile::EsmFile(QString fileName, ModelItem *parent)
                : ModelItem(parent), mFileName(fileName), mFormat(0)
{}

void ContentSelectorModel::EsmFile::setFileName(const QString &fileName)
{
    mFileName = fileName;
}

void ContentSelectorModel::EsmFile::setAuthor(const QString &author)
{
    mAuthor = author;
}

void ContentSelectorModel::EsmFile::setDate(const QDateTime &modified)
{
    mModified = modified;
}

void ContentSelectorModel::EsmFile::setFormat(int format)
{
    mFormat = format;
}

void ContentSelectorModel::EsmFile::setFilePath(const QString &path)
{
    mPath = path;
}

void ContentSelectorModel::EsmFile::setGameFiles(const QStringList &gamefiles)
{
    mGameFiles = gamefiles;
}

void ContentSelectorModel::EsmFile::setDescription(const QString &description)
{
    mDescription = description;
}

QByteArray ContentSelectorModel::EsmFile::encodedData() const
{
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    stream << mFileName << mAuthor << QString::number(mFormat)
           << mModified.toString() << mPath << mDescription
           << mGameFiles;

    return encodedData;
}

bool ContentSelectorModel::EsmFile::isGameFile() const
{ 
    return (mGameFiles.size() == 0) &&
        (mFileName.endsWith(QLatin1String(".esm"), Qt::CaseInsensitive) || 
        mFileName.endsWith(QLatin1String(".omwgame"), Qt::CaseInsensitive));
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
        return mFormat;
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

    case FileProperty_GameFile:
        return mGameFiles;
        break;

    default:
        break;
    }
    return QVariant();
}
void ContentSelectorModel::EsmFile::setFileProperty (const FileProperty prop, const QString &value)
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
        mFormat = value.toInt();
        break;

    case FileProperty_DateModified:
        mModified = QDateTime::fromString(value);
        break;

    case FileProperty_FilePath:
        mPath = value;
        break;

    case FileProperty_Description:
        mDescription = value;
        break;

    case FileProperty_GameFile:
        mGameFiles << value;
        break;

    default:
        break;
    }
}
